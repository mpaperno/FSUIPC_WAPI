#include "WASMIFCS.h"

using namespace FSUIPC::WASM::API;

WASMIFCS::WASMIFCS(int startEventNo)
{
	if (startEventNo < 0x11000 || startEventNo > 0x1FFF0)  // not really sure why this range... it's in  https://github.com/jldowson/FSUIPC_WDLL/blob/master/FSUIPC_WDLL/FSUIPC_WAPI.cpp#L7
		startEventNo = EVENT_START_NO;

	m_cbLogger = gcnew LogCallback(this, &WASMIFCS::onLoggerMessage);
	IntPtr pCb = Marshal::GetFunctionPointerForDelegate(m_cbLogger);
	m_hWasm = WASMIF::GetInstance(startEventNo, static_cast<LoggerCbType>(pCb.ToPointer()));

	// Register for a callback function that is called once all lvar / hvar CDAs have been loaded and are available
	m_cbUpdate = gcnew UpdateCallback(this, &WASMIFCS::onVarUpdate);
	pCb = Marshal::GetFunctionPointerForDelegate(m_cbUpdate);
	m_hWasm->registerUpdateCallback(static_cast<VarsListUpdatedCbType>(pCb.ToPointer()));

	// Register for a callback to be received when lvar values changed. Note that only lvars flagged for this callback will be returned. A terminating elements of -1 and -1.0 are added to each array returned.
	m_cbRecvList = gcnew ReceiveListCallback(this, &WASMIFCS::onValuesChanged);
	pCb = Marshal::GetFunctionPointerForDelegate(m_cbRecvList);
	m_hWasm->registerLvarUpdateCallback(static_cast<LVarsChangedByIdCbType>(pCb.ToPointer()));

	// As above bit returns the lvar the lvar name instead of the id, with the terminating element being NULL.
	m_cbRecvNamedList = gcnew ReceiveNamedListCallback(this, &WASMIFCS::onValuesChanged);
	pCb = Marshal::GetFunctionPointerForDelegate(m_cbRecvNamedList);
	m_hWasm->registerLvarUpdateCallback(static_cast<LVarsChangedByNameCbType>(pCb.ToPointer()));
}

WASMIFCS::~WASMIFCS()
{
	if (m_hWasm && m_hWasm->isRunning())
		m_hWasm->end();
	delete m_valueChangeHandler;
	delete m_namedValueChangedHandler;
	delete m_valuesChangedHandler;
	delete m_namedValuesChangeHandler;
	delete m_mc;
	m_hWasm = nullptr;  // our pointer is to, effectively, a static instance. d'tor is protected.
}

// static
inline WASMIFCS ^ WASMIFCS::GetInstance(int startEventNo)
{
	if (!m_instance)
		m_instance = gcnew WASMIFCS(startEventNo);
	else if (m_instance->getStartingEventNo() != startEventNo)
		m_instance->onLoggerMessage("WASMIFCS  [ALARM]: The startEventNo value cannot be changed after already creating the first instance of WASMIF class.");
	return m_instance;
}

inline IReadOnlyDictionary<System::String^, double>^ WASMIFCS::getLvarValues()
{
	map<string, double> returnMap = map<string, double>();
	m_hWasm->getLvarValues(returnMap);
	return marshal_map_to_dict<String ^, double>(returnMap);
}

inline IReadOnlyDictionary<int, System::String^>^ WASMIFCS::getLvarList()
{
	auto returnMap = unordered_map<int, string>();
	m_hWasm->getLvarList(returnMap);
	return marshal_map_to_dict<int, String ^>(returnMap);
}

inline IReadOnlyDictionary<int, System::String^>^ WASMIFCS::getHvarList()
{
	auto returnMap = unordered_map<int, string>();
	m_hWasm->getHvarList(returnMap);
	return marshal_map_to_dict<int, String ^>(returnMap);
}

/// <summary> Gets the name of an lvar from an id. </summary>

inline String ^ WASMIFCS::getLvarNameFromId(int id)
{
	// The name MUST be allocated to hold a minimum of MAX_VAR_NAME_SIZE (56) bytes
	char aName[MAX_VAR_NAME_SIZE] = { 0 };
	m_hWasm->getLvarNameFromId(id, aName);
	return marshal_as<String ^>(aName);
}

/// <summary> Gets the name of a hvar from an id. </summary>

inline String ^ WASMIFCS::getHvarNameFromId(int id)
{
	// The name MUST be allocated to hold a minimum of MAX_VAR_NAME_SIZE (56) bytes
	char aName[MAX_VAR_NAME_SIZE] = { 0 };
	m_hWasm->getHvarNameFromId(id, aName);
	return marshal_as<String ^>(aName);
}

inline void WASMIFCS::onValuesChanged(int id[], double values[])
{
	if (!m_valueChangeHandler && !m_valuesChangedHandler)
		return;

	auto ret = gcnew Dictionary<int, double>();
	int i = 0;
	while (id[i] != -1) {
		if (m_valuesChangedHandler)
			ret->Add(id[i], values[i]);
		if (m_valueChangeHandler)
			m_valueChangeHandler->Invoke(id[i], values[i]);
		++i;
	}
	if (i && m_valuesChangedHandler)
		m_valuesChangedHandler->Invoke(ret);

}

inline void WASMIFCS::onValuesChanged(const char ** names, double values[])
{
	if (!m_namedValueChangedHandler && !m_namedValuesChangeHandler)
		return;

	auto ret = gcnew Dictionary<System::String ^, double>();
	int i = 0;
	while (names[i] != nullptr) {
		auto name = gcnew System::String(names[i]);
		if (m_namedValuesChangeHandler)
			ret->Add(name, values[i]);
		if (m_namedValueChangedHandler)
			m_namedValueChangedHandler->Invoke(name, values[i]);
		++i;
	}
	if (i && m_namedValuesChangeHandler)
		m_namedValuesChangeHandler->Invoke(ret);
}

/*
Another way we could raise the lists of changed vars in an event, which would be faster than
building a dict _if_ we knew the useful array length w/out looping it. Though we'd still need to loop
if someone wants the individual var change notification events On[Named]ValueChanged.

public ref class ValuesChangedArgs : public EventArgs
{
public:
cli::array<int> ^ aIds;
cli::array<double> ^ aValues;
ValuesChangedArgs(cli::array<int> ^ ids, cli::array<double> ^ values) : aIds(ids), aValues(values) {}
};

... in onValuesChanged:

++i;  // length
cli::array<int> ^ aIds = gcnew cli::array<int>(i);
cli::pin_ptr<int> pSrc = &id[0], pDst = &aIds[0];
int *src = pSrc, *dst = pDst;
std::copy(src, src + i, dst);

cli::array<double> ^ aVals = gcnew cli::array<double>(i);
cli::pin_ptr<double> pVsrc = &values[0], pVdst = &aVals[0];
double *vsrc = pVsrc, *vdst = pVdst;
std::copy(vsrc, vsrc + i, vdst);

_valuesChangedHandler(this, gcnew ValuesChangedArgs(aIds, aVals));
*/

#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <msclr/marshal.h>

#include "WASMIF.h"

#define DELEGATE_HANDLER(H, D)      \
	{                                 \
		void add(D ^ h) { H += h; }     \
		void remove(D ^ h) { H -= h; }  \
	}

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace msclr::interop;

#pragma unmanaged
typedef void (* LoggerCbType)(const char*);
typedef void (* VarsListUpdatedCbType)(void);
typedef void (* LVarsChangedByIdCbType)(int[], double[]);
typedef void (* LVarsChangedByNameCbType)(const char**, double[]);

#pragma managed

namespace FSUIPC::WASM::API
{
	/// <summary>
	/// Wrapper for the FSUIPC WASM Module API library.
	/// Used mostly like the original version, with some necessary adjustments for C#.
	/// Containers and strings are converted to the C# equivalents, or in the case
	/// of the value list change handlers (which originally send 2 arrays) they're converted to dictionaries.
	/// And of course we use events/delegates for callbacks.
	///
	/// The only real addition are the "on variable changed" events (by name or ID) which can report
	/// individual changed values vs. the whole list at once. We have to loop over the incoming arrays anyway,
	/// so this is basically a "free" bonus feature.
	/// </summary>
	public ref class WASMIFCS
	{

	public:
		/// <summary> Logging verbosity level, used with setLogLevel() method. </summary>
		enum class LOG_LEVEL : USHORT
		{
			DISABLE = 1,
			INFO = 2,
			BUFFER = 3,
			DEBUG = 4,
			TRACE = 5,
			ENABLE = 6,
		};

		delegate void LogEntryReceivedDelegate(System::String ^ logEntry);
		delegate void VarsListUpdatedDelegate();
		delegate void ValueChangedDelegate(int id, double value);
		delegate void NamedValueChangedDelegate(System::String ^ name, double value);
		delegate void ValuesChangedDelegate(IReadOnlyDictionary<int, double> ^ vars);
		delegate void NamedValuesChangedDelegate(IReadOnlyDictionary<System::String ^, double> ^ vars);

		/// <summary> The default starting event ID.  </summary>
		static const int EVENT_START_ID = EVENT_START_NO;

		/// <summary> Log entry has been received from the WASM plug in. </summary>
		event LogEntryReceivedDelegate ^ OnLogEntryReceived;
		/// <summary> The list of available variables has been updated (eg. when aircraft is loaded). </summary>
		event VarsListUpdatedDelegate ^ OnVarsListUpdated;

		/// <summary> An lVar with given ID has changed value. </summary>
		event ValueChangedDelegate ^ OnValueChanged
			DELEGATE_HANDLER(m_valueChangeHandler, ValueChangedDelegate);
		/// <summary> lVar value(s) have changed, keyed by ID. </summary>
		event ValuesChangedDelegate ^ OnValuesChanged
			DELEGATE_HANDLER(m_valuesChangedHandler, ValuesChangedDelegate);
		/// <summary> An lVar with given name has changed value. </summary>
		event NamedValueChangedDelegate ^ OnNamedValueChanged
			DELEGATE_HANDLER(m_namedValueChangedHandler, NamedValueChangedDelegate);
		/// <summary> lVar value(s) have changed, keyed by name. </summary>
		event NamedValuesChangedDelegate ^ OnNamedValuesChanged
			DELEGATE_HANDLER(m_namedValuesChangeHandler, NamedValuesChangedDelegate);

		/// <summary> Get the static instance of this class using the default starting event ID. </summary>
		static WASMIFCS ^ GetInstance() { return GetInstance(EVENT_START_ID); }
		/// <summary> Get the static instance of this class using a specific starting event ID. </summary>
		static WASMIFCS ^ GetInstance(int startEventNo);

		/// <summary> Starts the connection to the WASM. </summary>
		bool start() { return m_hWasm->start(); }
		/// <summary> Returns True if connected to the WASM. </summary>
		bool isRunning() { return m_hWasm && m_hWasm->isRunning(); }
		/// <summary> Terminates the connection to the WASM. </summary>
		void end() { m_hWasm->end(); }
		/// <summary> This sends a request to the WASM to re-scan for lvars and hvars, and drop/recreate the CDAs. </summary>
		void reload() { m_hWasm->reload(); }
		/// <summary> Sets the lvar update frequency in the client. This must be called before start, and only takes affect if lvar update is disabled in the WASM module. </summary>
		void setLvarUpdateFrequency(int freq) { m_hWasm->setLvarUpdateFrequency(freq); }
		/// <summary> Used to set the SomConnect connection number to be used, from your SimConnect.cfg file. </summary>
		void setSimConfigConnection(int index) { m_hWasm->setSimConfigConnection(index); }
		/// <summary> Returns the lvar update frequency set in the client. </summary>
		int getLvarUpdateFrequency() { return m_hWasm->getLvarUpdateFrequency(); }
		/// <summary> Changes the log level used. </summary>
		void setLogLevel(LOG_LEVEL logLevel) { m_hWasm->setLogLevel((LOGLEVEL)logLevel); }
		/// <summary> Returns the starting event ID which this instance was created with. </summary>
		int getStartingEventNo() { return m_hWasm->getStartingEventNo(); }

		/// <summary> Returns an lvar value by ID. </summary>
		double getLvar(int lvarID) { return m_hWasm->getLvar(lvarID); }
		/// <summary> Returns an lvar value by name. Note that the name should NOT be prefixed by 'L:'. </summary>
		double getLvar(String ^ lvarName) { return m_hWasm->getLvar(m_mc->marshal_as<const char*>(lvarName)); }

		/// <summary> Convenience function. Sets the lvar value by id. The value is parsed and then either the short, unsigned short or double setLvar function is used. </summary>
		void setLvar(unsigned short id, String ^ value) { m_hWasm->setLvar(id, m_mc->marshal_as<const char*>(value)); }
		/// <summary> Sets the value of an lvar by id as a double. This request goes via a CDA. </summary>
		void setLvar(unsigned short id, double value) { m_hWasm->setLvar(id, value); }
		/// <summary> Sets the value of an lvar by id as a (signed) short. This request goes via an event. </summary>
		void setLvar(unsigned short id, short value) { m_hWasm->setLvar(id, value); }
		/// <summary> Sets the value of an lvar by id as an unsigned short. This request goes via an event. </summary>
		void setLvar(unsigned short id, unsigned short value) { m_hWasm->setLvar(id, value); }
		/// <summary> Convenience function. Sets the lvar value by name. The value is parsed and then either the short, unsigned short or double setLvar function is used. </summary>
		void setLvar(String ^ lvarName, String ^ value) { m_hWasm->setLvar(m_mc->marshal_as<const char*>(lvarName), m_mc->marshal_as<const char*>(value)); }
		/// <summary> Sets the value of an lvar by name as a double. </summary>
		void setLvar(String ^ lvarName, double value) { m_hWasm->setLvar(m_mc->marshal_as<const char*>(lvarName), value); }
		/// <summary> Sets the value of an lvar by name as a (signed) short. </summary>
		void setLvar(String ^ lvarName, short value) { m_hWasm->setLvar(m_mc->marshal_as<const char*>(lvarName), value); }
		/// <summary> Sets the value of an lvar by name as an unsigned short. </summary>
		void setLvar(String ^ lvarName, unsigned short value) { m_hWasm->setLvar(m_mc->marshal_as<const char*>(lvarName), value); }

		/// <summary> Activates a HTML variable by ID. </summary>
		void setHvar(int id) { m_hWasm->setHvar(id); }
		/// <summary> Activates a HTML variable by name. Note that, unlike lvars, the hvar name must be preceded by 'H:'. </summary>
		void setHvar(String ^ hvarName) { m_hWasm->setHvar(m_mc->marshal_as<const char*>(hvarName)); }
		/// <summary> Logs all lvars and values (to the defined logger). </summary>
		void logLvars() { m_hWasm->logLvars(); }
		/// <summary> Just print to log for now </summary>
		void logHvars() { m_hWasm->logHvars(); }
		/// <summary> Creates an lvar and sets its initial value. The lvar name should NOT be preceded by 'L:'. </summary>
		bool createLvar(String ^ lvarName, double value) { return m_hWasm->createLvar(m_mc->marshal_as<const char*>(lvarName), value); }
		/// <summary> Executes the argument calculator code. Max allowed length of the code is defined in the WASM.h by MAX_CALC_CODE_SIZE. </summary>
		void executeCalclatorCode(String ^ code) { m_hWasm->executeCalclatorCode(m_mc->marshal_as<const char*>(code)); }
		/// <summary> Flags an lvar to be included in the lvarUpdateCallback, by ID. Recommended to be used in your UpdateCallback. </summary>
		void flagLvarForUpdateCallback(int lvarId) { m_hWasm->flagLvarForUpdateCallback(lvarId); }
		/// <summary> Flags an lvar to be included in the lvarUpdateCallback, by name. Recommended to be used in your UpdateCallback. </summary>
		void flagLvarForUpdateCallback(String ^ lvarName) { m_hWasm->flagLvarForUpdateCallback(m_mc->marshal_as<const char*>(lvarName)); }

		/// <summary> Returns a dict of all lvar values keyed on the lvar name </summary>
		IReadOnlyDictionary<System::String ^, double> ^ getLvarValues();
		/// <summary> Returns a dict of lvar names keyed on the id </summary>
		IReadOnlyDictionary<int, System::String ^> ^ getLvarList();
		/// <summary> Returns a dict of hvar names keyed on the id </summary>
		IReadOnlyDictionary<int, System::String ^> ^ getHvarList();

		/// <summary> Utility function to get the id of an lvar. Lvar name must not be preceded by 'L:'. -1 returned if lvar not found </summary>
		int getLvarIdFromName(String ^ lvarName) { return m_hWasm->getLvarIdFromName(m_mc->marshal_as<const char*>(lvarName)); }
		/// <summary> Utility function to get the id of a hvar. Hvar name must be preceded by 'H:'. -1 returned if hvar not found </summary>
		int getHvarIdFromName(String ^ hvarName) { return m_hWasm->getHvarIdFromName(m_mc->marshal_as<const char*>(hvarName)); }

		/// <summary> Gets the name of an lvar from an id. </summary>
		String ^ getLvarNameFromId(int id);
		/// <summary> Gets the name of a hvar from an id. </summary>
		String ^ getHvarNameFromId(int id);

		~WASMIFCS();

	private:
		delegate void LogCallback(const char*);
		delegate void UpdateCallback();
		delegate void ReceiveListCallback(int[], double[]);
		delegate void ReceiveNamedListCallback(const char*[], double[]);

		static WASMIFCS ^ m_instance = nullptr;
		WASMIF* m_hWasm = nullptr;
		marshal_context ^ m_mc = gcnew marshal_context();
		LogCallback ^ m_cbLogger;
		UpdateCallback ^ m_cbUpdate;
		ReceiveListCallback ^ m_cbRecvList;
		ReceiveNamedListCallback ^ m_cbRecvNamedList;
		ValueChangedDelegate ^ m_valueChangeHandler;
		NamedValueChangedDelegate ^ m_namedValueChangedHandler;
		ValuesChangedDelegate ^ m_valuesChangedHandler;
		NamedValuesChangedDelegate ^ m_namedValuesChangeHandler;

		WASMIFCS(int startEventNo);
		void onLoggerMessage(const char* logString) { OnLogEntryReceived(gcnew System::String(logString)); }
		void onVarUpdate(void) { OnVarsListUpdated(); }
		void onValuesChanged(int id[], double values[]);
		void onValuesChanged(const char** names, double values[]);

		template<typename Kout, typename Vout, typename Map>
		inline IReadOnlyDictionary<Kout, Vout> ^ marshal_map_to_dict(const Map &from)
		{
			auto ret = gcnew Dictionary<Kout, Vout>((int)from.size());
			Kout key;
			Vout val;
			for (auto &kvp: from) {
				if constexpr (std::is_same_v<Kout, String ^>)
					key = marshal_as<Kout>(kvp.first.c_str());
				else
					key = kvp.first;
				if constexpr (std::is_same_v<Vout, String ^>)
					val = marshal_as<Vout>(kvp.second.c_str());
				else
					val = kvp.second;
				ret->Add(key, val);
			}
			return ret;
		}

	};
}

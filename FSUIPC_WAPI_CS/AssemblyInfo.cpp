
using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::CompilerServices;
using namespace System::Runtime::InteropServices;
using namespace System::Security::Permissions;

#ifdef _DEBUG
[assembly:AssemblyTitleAttribute(L"FSUIPC WASMIF C# Wrapper [DBG]")];
[assembly:AssemblyConfigurationAttribute("Debug")]
#else
[assembly:AssemblyTitleAttribute(L"FSUIPC WASMIF C# Wrapper")];
[assembly:AssemblyConfigurationAttribute("Release")]
#endif

[assembly:AssemblyDescriptionAttribute(L"C# Wrapper for PSUIPC WASM InterFace.")];
[assembly:AssemblyCompanyAttribute(L"Maxim Paperno")];
[assembly:AssemblyProductAttribute(L"FSUIPC_WAPI_CS")];
[assembly:AssemblyCopyrightAttribute(L"Copyright FSUIPC WASM Project Contributors")];
[assembly:AssemblyTrademarkAttribute(L"")];
[assembly:AssemblyCultureAttribute(L"")];

[assembly:AssemblyVersionAttribute("1.0.*")];

[assembly:ComVisible(false)];

// #include <node.h>
// #include <v8.h>
// #include <windows.h> // Per Windows (LoadLibrary, GetProcAddress)
// #include <string>

// namespace demo
// {

//     using v8::Array;
//     using v8::Context;
//     using v8::Exception;
//     using v8::Function;
//     using v8::FunctionCallbackInfo;
//     using v8::HandleScope;
//     using v8::Isolate;
//     using v8::Local;
//     using v8::Object;
//     using v8::String;
//     using v8::Value;

//     // Funzione per caricare la libreria dinamica e chiamare le funzioni
//     void LoadDLL(const FunctionCallbackInfo<Value> &args)
//     {
//         Isolate *isolate = args.GetIsolate();
//         HandleScope scope(isolate);

//         // Controlla se sono stati passati i parametri corretti
//         if (args.Length() < 4 || !args[0]->IsFunction() || !args[1]->IsNumber() || !args[2]->IsNumber() || !args[3]->IsArray())
//         {
//             isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Sono richiesti un callback, productId, vendorId e un array di dati").ToLocalChecked()));
//             return;
//         }

//         // Ottieni il callback
//         Local<Function> callback = Local<Function>::Cast(args[0]);

//         // Ottieni productId e vendorId
//         unsigned short productId = static_cast<unsigned short>(args[1]->Uint32Value(isolate->GetCurrentContext()).ToChecked());
//         unsigned short vendorId = static_cast<unsigned short>(args[2]->Uint32Value(isolate->GetCurrentContext()).ToChecked());

//         // Ottieni i dati del report
//         Local<Array> inputData = Local<Array>::Cast(args[3]); // Cambia l'indice a 3 per l'array di dati
//         const size_t reportLength = inputData->Length();
//         unsigned char *reportData = new unsigned char[reportLength]; // Dichiarazione di reportData
//         for (size_t i = 0; i < reportLength; i++)
//         {
//             reportData[i] = static_cast<unsigned char>(inputData->Get(isolate->GetCurrentContext(), i).ToLocalChecked()->Uint32Value(isolate->GetCurrentContext()).ToChecked());
//         }

//         // Controlla se sono stati passati i parametri corretti
//         if (args.Length() < 2 || !args[0]->IsFunction() || !args[1]->IsArray())
//         {
//             isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Sono richiesti un callback e un array di dati").ToLocalChecked()));
//             return;
//         }

//         // Percorso della DLL (puoi passare il percorso come argomento)
//         std::string dllPath = "hidapi.dll"; // Cambia il nome della DLL se necessario

//         // Carica la DLL
//         HMODULE hModule = LoadLibrary(dllPath.c_str());
//         if (!hModule)
//         {
//             std::string errorMessage = "Errore: impossibile caricare la DLL " + dllPath;
//             Local<Value> argv[] = {String::NewFromUtf8(isolate, errorMessage.c_str()).ToLocalChecked()};
//             callback->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv).ToLocalChecked();
//             delete[] reportData;
//             return;
//         }

//         // Dichiarazione dei tipi di funzione
//         typedef int (*InitFunctionType)();
//         typedef int (*ExitFunctionType)();
//         typedef void *(*OpenFunctionType)(unsigned short, unsigned short, const wchar_t *);
//         typedef int (*SendFeatureReportFunctionType)(void *, const unsigned char *, size_t);
//         typedef int (*CloseFunctionType)(void *);

//         // Ottieni gli indirizzi delle funzioni
//         FARPROC initFunction = GetProcAddress(hModule, "hid_init");
//         FARPROC exitFunction = GetProcAddress(hModule, "hid_exit");
//         FARPROC openFunction = GetProcAddress(hModule, "hid_open");
//         FARPROC sendFeatureReportFunction = GetProcAddress(hModule, "hid_send_feature_report");
//         FARPROC closeFunction = GetProcAddress(hModule, "hid_close");

//         if (!initFunction || !exitFunction || !openFunction || !sendFeatureReportFunction || !closeFunction)
//         {
//             std::string errorMessage = "Errore: impossibile trovare una o più funzioni nella DLL " + dllPath;
//             Local<Value> argv[] = {String::NewFromUtf8(isolate, errorMessage.c_str()).ToLocalChecked()};
//             callback->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv).ToLocalChecked();
//             FreeLibrary(hModule); // Libera la DLL
//             delete[] reportData;
//             return;
//         }

//         // Cast dei puntatori a funzione
//         InitFunctionType init = reinterpret_cast<InitFunctionType>(initFunction);
//         ExitFunctionType exit = reinterpret_cast<ExitFunctionType>(exitFunction);
//         OpenFunctionType open = reinterpret_cast<OpenFunctionType>(openFunction);
//         SendFeatureReportFunctionType sendFeatureReport = reinterpret_cast<SendFeatureReportFunctionType>(sendFeatureReportFunction);
//         CloseFunctionType close = reinterpret_cast<CloseFunctionType>(closeFunction);

//         // Chiama hid_init
//         int result = init();
//         if (result != 0)
//         {
//             std::string errorMessage = "Errore: la funzione 'hid_init' ha restituito un errore";
//             Local<Value> argv[] = {String::NewFromUtf8(isolate, errorMessage.c_str()).ToLocalChecked()};
//             callback->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv).ToLocalChecked();
//             FreeLibrary(hModule); // Libera la DLL
//             delete[] reportData;
//             return;
//         }

//         // Chiama hid_open (esempio con un dispositivo specifico)
//         void *deviceHandle = open(0x1234, 0x5678, NULL); // Cambia con i valori corretti
//         if (!deviceHandle)
//         {
//             std::string errorMessage = "Errore: la funzione 'hid_open' ha restituito NULL";
//             Local<Value> argv[] = {String::NewFromUtf8(isolate, errorMessage.c_str()).ToLocalChecked()};
//             callback->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv).ToLocalChecked();
//             exit();               // Chiama hid_exit
//             FreeLibrary(hModule); // Libera la DLL
//             delete[] reportData;
//             return;
//         }

//         // Chiama hid_send_feature_report
//         int sendResult = sendFeatureReport(deviceHandle, reportData, reportLength);
//         if (sendResult < 0)
//         {
//             std::string errorMessage = "Errore: la funzione 'hid_send_feature_report' ha restituito un errore";
//             Local<Value> argv[] = {String::NewFromUtf8(isolate, errorMessage.c_str()).ToLocalChecked()};
//             callback->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv).ToLocalChecked();
//             close(deviceHandle);  // Chiama hid_close
//             exit();               // Chiama hid_exit
//             FreeLibrary(hModule); // Libera la DLL
//             delete[] reportData;
//             return;
//         }

//         // Chiama hid_close
//         close(deviceHandle);

//         // Chiama hid_exit
//         exit();

//         // Libera la DLL
//         FreeLibrary(hModule);
//         delete[] reportData;

//         // Restituisci un messaggio di successo
//         std::string successMessage = "DLL caricata e funzioni chiamate con successo. Report inviato correttamente.";
//         Local<Value> argv[] = {String::NewFromUtf8(isolate, successMessage.c_str()).ToLocalChecked()};
//         callback->Call(isolate->GetCurrentContext(), isolate->GetCurrentContext()->Global(), 1, argv).ToLocalChecked();
//     }

//     // Funzione di inizializzazione dell'addon
//     void Initialize(Local<Object> exports)
//     {
//         NODE_SET_METHOD(exports, "loadDLL", LoadDLL); // Registra la funzione loadDLL
//     }

//     NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

// } // namespace demo
#include <node.h>
#include <v8.h>
#include <windows.h> 
#include <string>

namespace demo
{
    using v8::Array;
    using v8::Context;
    using v8::Exception;
    using v8::Function;
    using v8::FunctionCallbackInfo;
    using v8::HandleScope;
    using v8::Isolate;
    using v8::Local;
    using v8::Object;
    using v8::String;
    using v8::Value;
    using v8::Number;
    using v8::Null; 

    void LoadDLL(const FunctionCallbackInfo<Value> &args)
    {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        if (args.Length() < 4 || !args[0]->IsFunction() || !args[1]->IsNumber() || !args[2]->IsNumber() || !args[3]->IsArray())
        {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Sono richiesti un callback, productId, vendorId e un array di dati").ToLocalChecked()));
            return;
        }

        Local<Function> callback = Local<Function>::Cast(args[0]);
        int productId = args[1]->NumberValue(isolate->GetCurrentContext()).ToChecked();
        int vendorId = args[2]->NumberValue(isolate->GetCurrentContext()).ToChecked();
        Local<Array> dataArray = Local<Array>::Cast(args[3]);

        HINSTANCE hinstLib = LoadLibraryA("./hidapi.dll");

        if (hinstLib == NULL)
        {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Errore durante il caricamento della libreria").ToLocalChecked()));
            return;
        }

        FARPROC pfnProc = GetProcAddress(hinstLib, "YourFunctionName");

        if (pfnProc == NULL)
        {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Errore durante il recupero del puntatore alla funzione").ToLocalChecked()));
            return;
        }

        unsigned char* reportData = new unsigned char[dataArray->Length()];
        for (size_t i = 0; i < dataArray->Length(); ++i) {
            reportData[i] = static_cast<unsigned char>(dataArray->Get(isolate->GetCurrentContext(), i).ToLocalChecked()->NumberValue(isolate->GetCurrentContext()).ToChecked());
        }

        int result = ((int (*)(int, int, unsigned char *))pfnProc)(productId, vendorId, reportData);

        delete[] reportData;

        Local<Value> argv[] = {
            Number::New(isolate, result)
        };

        callback->Call(isolate->GetCurrentContext(), Null(isolate), 1, argv);

        FreeLibrary(hinstLib);
    }

    void Initialize(Local<Object> exports)
    {
        NODE_SET_METHOD(exports, "loadDLL", LoadDLL);
    }

    NODE_MODULE(addon, Initialize)

} // namespace demo
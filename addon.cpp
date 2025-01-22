#include "node_modules\node-addon-api\napi.h"
#include "./hidapi/hidapi/hidapi.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
struct HIDField {
    unsigned char tag;
    unsigned char type;
    unsigned char size;
    unsigned char data;
};

// Funzione per analizzare il descrittore HID
void parseHIDReportDescriptor(const unsigned char* descriptor, int length, int& numButtons, int& numAxes) {
    numButtons = 0;
    numAxes = 0;

    std::vector<HIDField> fields;
    int index = 0;

    // Decodifica il descrittore HID
    while (index < length) {
        HIDField field;
        field.tag = descriptor[index] & 0xF0; // Estrae il tag (4 bit alti)
        field.type = (descriptor[index] & 0x0C) >> 2; // Estrae il tipo (2 bit centrali)
        field.size = descriptor[index] & 0x03; // Estrae la dimensione (2 bit bassi)

        // Determina la dimensione effettiva del campo
        if (field.size == 0x03) {
            field.size = descriptor[index + 1]; // Dimensione variabile
            index += 2;
        } else {
            field.size = (field.size == 0x00) ? 1 : (field.size == 0x01) ? 2 : 4;
            index++;
        }

        // Leggi i dati del campo
        field.data = descriptor[index];
        fields.push_back(field);
        index++;
    }

    // Analizza i campi per determinare il numero di pulsanti e assi
    for (const auto& field : fields) {
        if (field.type == 0x01) { // Tipo "Main"
            switch (field.tag) {
                case 0x80: // Input
                case 0x90: // Output
                case 0xB0: // Feature
                    // Controlla i bit del campo per determinare pulsanti e assi
                    if (field.data & 0x02) { // Bit di pulsante
                        numButtons += field.size * 8; // Ogni bit rappresenta un pulsante
                    }
                    if (field.data & 0x04) { // Bit di asse
                        numAxes += field.size; // Ogni byte rappresenta un asse
                    }
                    break;

                default:
                    break;
            }
        }
    }
}


Napi::Value getControllerData(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    // Verifica che l'input sia valido
    if (info.Length() < 1 || !info[0].IsObject())
    {
        Napi::TypeError::New(env, "Expected an object as argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Recupera l'oggetto con i dati del controller
    Napi::Object obj = info[0].As<Napi::Object>();
    int productId = obj.Get("productId").As<Napi::Number>().Int32Value();
    int vendorId = obj.Get("vendorId").As<Napi::Number>().Int32Value();
    std::string product = obj.Has("product") ? obj.Get("product").As<Napi::String>().Utf8Value() : "Unknown Product";

    // Log iniziale
    std::cout << "[INFO C++] Controller ricevuto:\n"
              << "  Product ID: " << productId << "\n"
              << "  Vendor ID: " << vendorId << "\n"
              << "  Product: " << product << std::endl;

    // Inizializza HIDAPI
    if (hid_init() != 0)
    {
        Napi::TypeError::New(env, "Failed to initialize HIDAPI").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Enumerazione per controllare il percorso (USB o Bluetooth)
    struct hid_device_info *device_info = hid_enumerate(vendorId, productId);
    bool isBluetooth = false;
    const char *device_path = nullptr;

    while (device_info)
    {
        if (device_info->vendor_id == vendorId && device_info->product_id == productId)
        {
            device_path = device_info->path;
            // Controlla se il percorso indica una connessione Bluetooth
            if (device_path && std::string(device_path).find("bluetooth") != std::string::npos)
            {
                isBluetooth = true;
                break;
            }
        }
        device_info = device_info->next;
    }
    hid_free_enumeration(device_info);

    if (!device_path)
    {
        Napi::TypeError::New(env, "Failed to locate HID device").ThrowAsJavaScriptException();
        hid_exit();
        return env.Null();
    }

    std::cout << "[INFO C++] Device Path: " << device_path << std::endl;
    if (isBluetooth)
        std::cout << "[INFO C++] Device is connected via Bluetooth." << std::endl;
    else
        std::cout << "[INFO C++] Device is connected via USB." << std::endl;

    // Apri il dispositivo HID
    hid_device *device = hid_open(vendorId, productId, nullptr);
    if (!device)
    {
        hid_exit();
        Napi::TypeError::New(env, "Failed to open HID device").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Lettura dei dati dal dispositivo
    unsigned char buf[1024];
    int res = hid_read(device, buf, sizeof(buf));
    if (res < 0)
    {
        std::cerr << "Errore nella lettura del report di input. Codice di errore: " << res << std::endl;
        hid_close(device);
        hid_exit();
        return env.Null();
    }

    // Log del report ricevuto
    std::cerr << "[DEBUG] Feature report length: " << res << std::endl;
    std::cerr << "[DEBUG] Data in report: ";
    for (int i = 0; i < res; i++)
    {
        std::cerr << "0x" << std::hex << (int)buf[i] << " ";
    }
    std::cerr << std::endl;

    // Ottieni il descrittore HID
    unsigned char descriptor[255];  // Un buffer abbastanza grande
    int descriptor_length = hid_get_report_descriptor(device, descriptor, sizeof(descriptor));  // Ottieni il descrittore del report
    if (descriptor_length < 0)
    {
        std::cerr << "Errore nel recupero del descrittore HID. Codice di errore: " << descriptor_length << std::endl;
        hid_close(device);
        hid_exit();
        return env.Null();
    }

    std::cerr << "[DEBUG] HID descriptor (length: " << descriptor_length << "): ";
    for (int i = 0; i < descriptor_length; i++)
    {
        std::cerr << "0x" << std::hex << (int)descriptor[i] << " ";
    }
    std::cerr << std::endl;

    // Variabili per il conteggio di pulsanti e assi
    int numButtons = 0;
    int numAxes = 0;

    // Analizza il descrittore HID
    parseHIDReportDescriptor(descriptor, descriptor_length, numButtons, numAxes);

    // Chiudi il dispositivo e termina HIDAPI
    hid_close(device);
    hid_exit();

    // Restituisci la risposta
    std::string response = "Controller elaborato con successo via " + std::string(isBluetooth ? "Bluetooth" : "USB") + ": " + product + 
                           " (Pulsanti: " + std::to_string(numButtons) + ", Assi: " + std::to_string(numAxes) + ")";
    return Napi::String::New(env, response);
}

// Funzione di inizializzazione del modulo
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set("getControllerData", Napi::Function::New(env, getControllerData));
    return exports;
}

// Macro per definire il modulo
NODE_API_MODULE(addon, Init)

const myAddon = require('./build/Release/myAddon');
const http = require('http');
const port = 3000;
const address = 'localhost';


async function getControllerData(controller) {
    try {
        console.log('Caricamento dell\'addon...');

        if (!controller.productId || !controller.vendorId) {
            console.error('[ERRORE] Controller non compatibile: manca productId o vendorId.');
            return 'Controller non compatibile';
        }

        //Passa i dati del controller all'addon e ottieni il risultato
        const result = myAddon.getControllerData({
            productId: controller.productId,
            vendorId: controller.vendorId,
            product: controller.productName || "Unknown Product",
        });

        console.log('[INFO] Risultato dall\'addon:', result); 
        return result; // Restituisce il risultato all'handler HTTP

    } catch (error) {
        console.error('[ERRORE] Durante l\'esecuzione dell\'addon:', error);
        return 'Errore durante l\'esecuzione dell\'addon';
    }
}

// Crea il server HTTP
const server = http.createServer((req, res) => {
    console.log(`Client connected: ${req.socket.remoteAddress}`); // Stampa l'indirizzo IP del client

    // Imposta gli header CORS
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    // Accumula i dati ricevuti
    let body = '';

    // Ascolta i dati in arrivo
    req.on('data', chunk => {
        body += chunk.toString(); // Concatena i dati ricevuti
    });

    // Quando tutti i dati sono stati ricevuti
    req.on('end', async () => {
        try {
            const jsonData = JSON.parse(body); // Prova a fare il parsing del JSON

            // Gestisci le richieste in base all'URL
            if (req.url === '/' && req.method === 'POST') {
                res.statusCode = 200; // Imposta il codice di stato della risposta
                res.setHeader('Content-Type', 'application/json'); // Imposta il tipo di contenuto

                // Estrai i dati del controller dal JSON
                const controller = {
                    productName: jsonData[0]?.productName,
                    productId: jsonData[0]?.productId,
                    vendorId: jsonData[0]?.vendorId
                };

                // Chiama la funzione per invocare l'addon
                const addonResult = await getControllerData(controller);

                // Invia la risposta al client con i dati dell'addon
                res.end(JSON.stringify({
                    message: 'JSON received!',
                    addonResult: addonResult, // Dati ritornati dall'addon
                    data: jsonData
                }));
            } else {
                res.statusCode = 404; // Codice di stato per "Non trovato"
                res.end('Not Found'); // Risposta di errore
            }
        } catch (error) {
            console.error('Error parsing JSON:', error);
            res.statusCode = 400; // Codice di stato per "Richiesta non valida"
            res.end('Invalid JSON'); // Risposta di errore
        }
    });
});

// Gestisci la preflight request per CORS
server.on('options', (req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    res.statusCode = 204; // No Content
    res.end();
});

// Gestisci errori del server
server.on('error', (err) => {
    if (err.code === 'EADDRINUSE') {
        console.error(`Port ${port} is already in use. Please use a different port.`);
        process.exit(1);
    } else {
        console.error('Server error:', err);
    }
});

// Avvia il server
server.listen(port, () => {
    console.log(`Server running at http://${address}:${port}`);
});

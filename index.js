const addon = require('./build/Release/addon.node');
const http = require('http');
const port = 3001;
const HID = require('node-hid');

let previousControllers = [];
let selectedController = null;

const server = http.createServer((req, res) => {
    if (req.url === '/controllers' && req.method === 'GET') {
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify(previousControllers));
    } else {
        res.statusCode = 404;
        res.end('Not Found');
    }
});

server.on('error', (err) => {
    if (err.code === 'EADDRINUSE') {
        console.error(`Port ${port} is already in use. Please use a different port.`);
        process.exit(1);
    } else {
        console.error('Server error:', err);
    }
});

server.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
    detectControllers();
});

function detectControllers() {
    const hidDevices = HID.devices();
    const hidProducts = hidDevices.map((device) => {
        if (device.product && device.product !== 'undefined') {
            return {
                product: device.product,
                productId: device.productId,
                vendorId: device.vendorId
            };
        }
        return null;
    }).filter(product => product !== null);

    previousControllers = [...new Set(hidProducts)];
    selectController();
}

function selectController() {
    const rl = require('readline').createInterface({
        input: process.stdin,
        output: process.stdout,
    });

    console.log("\nAvailable controllers:");
    previousControllers.forEach((controller, index) => {
        console.log(`${index + 1}: ${controller.product} (Product ID: ${controller.productId}, Vendor ID: ${controller.vendorId})`);
    });

    rl.question('\nEnter the number of the controller to select: ', (answer) => {
        const index = parseInt(answer) - 1;

        if (index >= 0 && index < previousControllers.length) {
            selectedController = previousControllers[index];
            console.log(`\n[INFO] Selected controller: ${selectedController.product}`);
            getControllerData(selectedController);
        } else {
            console.log("\n[ERROR] Invalid selection. Please select a valid controller.");
        }

        rl.close();
    });
}

async function getControllerData(controller) {
    try {
        console.log('Caricamento dell\'addon...');
        const productId = controller.productId;
        const vendorId = controller.vendorId;

        // Crea un array di dati da inviare (esempio: un array di 64 byte)
        const dataArray = new Uint8Array(64); // Modifica la lunghezza e i valori come necessario

        addon.loadDLL(
            (result) => {
                console.log("Success:", result);
            },
            productId, 
            vendorId, 
            dataArray
        ); 
    } catch (error) {
        console.error('Errore durante l\'esecuzione dell\'addon:', error);
    }
}
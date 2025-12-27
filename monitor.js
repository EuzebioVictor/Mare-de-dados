const express = require('express');
const app = express();

app.use(express.json());

// Rota onde o Arduino vai entregar os dados
app.post('/api/dados', async (req, res) => {
    console.log("------------------------------------");
    
    // 1. Pega os dados que vieram do Arduino
    const dados = {
        sensor_id: req.body.sensor_id,
        temp: req.body.temp,
        umid: req.body.umid,
        co2: req.body.co2
    };

    console.log(`DADOS RECEBIDOS: { ID: ${dados.sensor_id}, Temp: ${dados.temp}°C, Umid: ${dados.umid}%, CO2: ${dados.co2} ppm }`);

    // 2. REPASSE PARA O GOOGLE SHEETShttps
    const googleUrl = ""; //URL GOOGLE SHEET

    try {
        // O fetch envia os dados para o Google
        const response = await fetch(googleUrl, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(dados)
        });

        if (response.ok) {
            console.log("SUCESSO: Enviado para o Google Sheets!");
        } else {
            console.log("AVISO: Google Sheets recusou (Status: " + response.status + ")");
        }
    } catch (error) {
        console.log("ERRO AO MANDAR PARA O GOOGLE:", error.message);
    }

    console.log("HORA:", new Date().toLocaleTimeString());
    res.status(200).send("Recebido localmente!");
});

app.listen(8080, '0.0.0.0', () => {
    console.log("SERVIDOR ONLINE!");
    console.log("Esperando dados do Arduino na porta 8080...");
});
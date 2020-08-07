var Commander = async function(args) { 
    const axios = require('axios');
    let ip = args;

    async function sendFace(expression) {
        try {
            let response = await axios.get(`http://${ip}/sendFace?expression=` + expression);
            console.log('Enviado com sucesso' + response);
        } catch(err) {
            console.log(err);
        }
    }

    async function getStatus() {
        try {
            let status = await axios.get(`http://${ip}/getStatus`);
            console.log("Robot Status: ");
            console.log(status.data);
            return status.data;
        } catch(err) {
            console.log(err);
            throw err;
        }
    }

    async function initRobot() {
        try {
            await axios.get(`http://${ip}/initRobot`);
            console.log('Iniciado com sucesso');
        } catch(err) {
            console.log(err);
        }
    }

    async function sendData(command) {
        try {
            await axios.get(`http://${ip}/sendCommand?command=` + command);
            console.log('Enviado com sucesso: ' + command);
        } catch(err) {
            console.log(err);
        }
    }


    async function sendSong() {
        try {
            await axios.get(`http://${ip}/playSong?song=` + getSongByteString('GAME_OF_THRONES'));
            console.log('Enviado com sucesso' + response);
        } catch(err) {
            console.log(err);
        }
    };

    return {
        sendFace,
        sendData,
        sendSong,
        initRobot,
        getStatus
    }
};



var decodeQuery = function(query) {
    switch(query) {
        case 'frente': 
            return 2;
        case 'tras':
            return 3;
        case 'direita':
            return 5;
        case 'esquerda':
            return 4;
        case 'dormir':
            return 8;
        case 'acordar':
            return 9;
        default:
            return 0;
    }
}

var decodeNumber = function(number) {
    switch(number) {
        case 'um':
            return 1;
        case 'dois':
            return 2;
        case 'tres':
            return 3;
        case 'quatro':
            return 4;
        case 'cinco':
            return 5;
        case 'seis':
            return 6;
        default:
            return 0;s
    }
}


module.exports = {
    Commander,
    decodeQuery,
    decodeNumber
}
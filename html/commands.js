var Commander = function() { 
    function sendFace(expression) {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
                console.log('Enviado sucesso');
            }
        };
        xhttp.open("GET", "http://192.168.1.72/sendFace?expression=" + expression, true);
        xhttp.send();
    }

    function initRobot() {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
                console.log('Enviado sucesso');
            }
        };
        xhttp.open("GET", "http://192.168.1.72/initRobot", true);
        xhttp.send();
    }

    function sendData(command) {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
                console.log('Enviado sucesso');
            }
        };
        xhttp.open("GET", "http://192.168.1.72/sendCommand?command=" + command, true);
        xhttp.send();
    }


    function sendSong() {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
                console.log('Enviado sucesso');
            }
        };
        xhttp.open("GET", "http://192.168.1.72/playSong?song=" + getSongByteString('GAME_OF_THRONES'), true);
        xhttp.send();
    };

    return {
        sendFace,
        sendData,
        sendSong,
        initRobot
    }
}();
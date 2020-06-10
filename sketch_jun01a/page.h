const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <style>
        .button-container {
            width: 500px;
            height: 300px;
            margin: 30px;
            border-radius:6px;
            border:1px solid #dcdcdc;
        }
        .video-container {
            width: 500px;
            height: 300px;
            margin: 30px;
            border-radius:6px;
            border:1px solid #dcdcdc;
            align-items: center;
            display: flex;
            justify-content: center;
        }
        .idle-active {
            width: 500px;
            margin: 20px;
        }
        .move {
            margin: 10px;
            width: 500px;
            display: flex;
        }
        .turn {
            margin: 10px;
            width: 200px;
        }
        .walk {
            margin: 10px;
            width: 200px;
        }
        button {
            margin: 5px;
            width: 85px;
            box-shadow:inset 0px 1px 0px 0px #ffffff;
            background:linear-gradient(to bottom, #ffffff 5%, #f6f6f6 100%);
            background-color:#ffffff;
            border-radius:6px;
            border:1px solid #dcdcdc;
            display:inline-block;
            cursor:pointer;
            color:#666666;
            font-family:Arial;
            font-size:12px;
            font-weight:bold;
            padding:6px 0px;
            text-decoration:none;
            text-shadow:0px 1px 0px #ffffff;
        }
        button:hover {
            background:linear-gradient(to bottom, #f6f6f6 5%, #ffffff 100%);
	        background-color:#f6f6f6;
        }
        button:active {
            position:relative;
	        top:1px;
        }
    </style>
    <body>
        <div id="button-container" class="button-container">
                <div class="idle-active">
                    <button type="button" onclick="sendData('0')">Levantar</button>
                    <button type="button" onclick="sendData('1')">Sentar</button><BR>
                </div>
                <div class="move">
                    <div class="walk">
                        <button type="button" onclick="sendData('2')">Frente</button><br>
                        <button type="button" onclick="sendData('3')">Tras</button>
                    </div>
                    <div class="turn">
                        <button type="button" onclick="sendData('4')">Esquerda</button>
                        <button type="button" onclick="sendData('5')">Direita</button><BR>
                    </div>
                </div>
            </div>
        <div class="video-container">
            <video id="video" width="320" height="240" autoplay muted>    
            </video>
            <canvas id="canvas" style="position: absolute;"></canvas>
        </div>
        <script>     
        function sendData(command) {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    console.log('Enviado sucesso');
                }
            };
            xhttp.open("GET", "sendCommand?command=" + command, true);
            xhttp.send();
        }
        </script>
        <script src="https://cdn.jsdelivr.net/npm/handtrackjs/dist/handtrack.min.js"> </script>
        <script>
            let handshakeQueue = function () {
                let queue = [];
                let isSending = false;

                let addToQueue = function(element) {
                    if(isSending == true) {
                        return;
                    } else {
                        queue.push(element);
                        sendToRobot();
                    }
                }

                let sendToRobot = async function () {
                    console.log(queue[0]);
                    isSending = true;
                    sendData('7');
                    setTimeout(function() {
                        isSending = false;
                    }, 5000);
                    queue.shift();
                }

                return {
                    addToQueue,
                }
            }();

            let model = null;
            let isVideo = true;
            const canvas = document.getElementById("canvas");
            const context = canvas.getContext("2d");
            const modelParams = {
                flipHorizontal: true,   // flip e.g for video  
                maxNumBoxes: 20,        // maximum number of boxes to detect
                iouThreshold: 0.5,      // ioU threshold for non-max suppression
                scoreThreshold: 0.94,    // confidence threshold for predictions.
            }
            const video = document.getElementById('video');

            function startVideo() { 
                handTrack.startVideo(video).then(status => {
                console.log("video started", status);
                    if (status) {
                        runDetection()
                    }
                });
            }

            async function runDetection() {
                model.detect(video).then(async predictions => {
                    if(predictions.length > 0) {
                        handshakeQueue.addToQueue(predictions[0]);
                    }
                    model.renderPredictions(predictions, canvas, context, video);
                    if (isVideo) {
                        requestAnimationFrame(runDetection);
                    }
                });
            }

            handTrack.load(modelParams).then(lmodel => {
                model = lmodel;
                startVideo();
            });

            function sleep(time) {
                return new Promise(function(resolve, reject) {
                    setTimeout(function() {
                        resolve(true);
                    }, time);
                });
            }
        </script>
    </body>
</html>
)=====";
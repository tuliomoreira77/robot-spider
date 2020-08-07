var mdns = require('mdns-js');
var browser = mdns.createBrowser();
const DbConn = require('./app/DbConn');
const CommandUtils = require('./app/Commander');
const { buildStatus } = require('./app/DbConn');
var ping = require('ping');
 
async function getCommand() {
    try {
        var command = await DbConn.CommandQueueDb.findOneAndDelete().sort({date: 1});
        console.log(command);
        return command;
    } catch(err) {
        console.log(err);
    }
}

async function poolingQueue() {
    try {
        let ip = await getRobotIp();//await findRobotIp();
        let Commander = await CommandUtils.Commander(ip);
        let count = 0;
        let initControl = {
            control: false,
        }
        await Commander.initRobot();
        while(true) {
            console.log('...');
            let command = await getCommand();
            if(command) {
                if(!initControl.control) {
                    await Commander.sendData('90');
                    initControl.control = true;
                }
                disableTimer = null;
                let decodedQuery = `${CommandUtils.decodeQuery(command.direction)}${command.steps}`;
                await Commander.sendData(decodedQuery);
            }
            if(count > 5) {
                count = 0;
                try {
                    let rawStatus = await Commander.getStatus();
                    let status = buildStatus(rawStatus);
                    await DbConn.RobotStatusDb.findOneAndUpdate({_id: "5f22372a5b3d4377b1fcb1f9"}, status);
                } catch(err) {
                    console.log('Nao foi possivel buscar o status do robô...');
                }
            }
            await wait(1000);
            count++;
        }
    } catch(err) {
        console.log(err);
    }
}

async function disableRobotOnTime(Commander, initControl, count) {
    console.log(`Inatividade detectada... Desligando Motores`);
    await Commander.sendData('80');
    initControl.control = false;
}

async function getRobotIp() {
    let host = ['spiderrobot.local'];
    let res = await ping.promise.probe(host, {
        timeout: 10,
        extra: ['-i', '2'],
    });
    return res.numeric_host;
}

function wait(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}


poolingQueue();
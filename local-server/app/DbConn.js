const mongoose = require('mongoose');
const dbURL = 'mongodb+srv://servico:Cg3EbRHiIih7FGF3@alcateiagerenciamento-erjo8.mongodb.net/robot-spider?retryWrites=true&w=majority';
mongoose.connect(dbURL, {
    useNewUrlParser: true,
    useFindAndModify: false,
    useCreateIndex: true,
    useUnifiedTopology: true
});

const robotStatusSchema = new mongoose.Schema({
    busy: Boolean,
    humor: {name: String, ordinal: Number},
    battery: Number,
    heartbeat: Number
});

const commandQueueSchema = new mongoose.Schema({
    date: Number,
    direction: String,
    steps: Number,
    sender: String,
    priority: Number,
});
const CommandQueueDb = mongoose.model('commandqueue', commandQueueSchema);
const RobotStatusDb = mongoose.model('status', robotStatusSchema);

function buildCommandQueueObject(obj, priority) {
    return {
        date: +new Date(),
        sender: 'Alexa',
        priority: priority ? 0 : priority,
        direction: obj.direction.value,
        steps: obj.steps.value
    }
}

function buildStatus(obj) {
    return {
        busy: obj.busy,
        battery: obj.battery,
        heartbeat: +new Date(),
    }
}


module.exports = {
    CommandQueueDb,
    RobotStatusDb,
    buildCommandQueueObject,
    buildStatus
}

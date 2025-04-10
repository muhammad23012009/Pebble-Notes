var Base64 = require('js-base64').Base64;

var appMessageQueue = {
    queue: [],
    numTries: 0,
    maxTries: 5,
    working: false,
    clear: function() {
        this.queue = [];
        this.numTries = 0;
        this.working = false;
    },
    isEmpty: function() {
        return this.queue.length === 0;
    },
    nextMessage: function() {
        return this.isEmpty() ? {} : this.queue[0];
    },
    send: function(message) {
        if (message) this.queue.push(message);
        if (this.working) return;
        if (this.queue.length > 0) {
            this.working = true;
            var ack = function() {
                appMessageQueue.numTries = 0;
                appMessageQueue.queue.shift();
                appMessageQueue.working = false;
                appMessageQueue.send();
            };
            var nack = function() {
                appMessageQueue.numTries++;
                appMessageQueue.working = false;
                appMessageQueue.send();
            };
            if (this.numTries >= this.maxTries) {
                console.log("[AppMessage] failed: " + JSON.stringify(this.nextMessage()));
                ack();
            }
            console.log("[AppMessage] sending: " + JSON.stringify(this.nextMessage()));
            Pebble.sendAppMessage(this.nextMessage(), ack, nack);
        }

    }
}

function decrementIndex(index)
{
    var count = getCount();
    if (index < count) {
        for (var i = index; i < count; i++) {
            localStorage.setItem(i, localStorage.getItem(i+1));
        }
    }
}

function getCount()
{
    var count = localStorage.getItem("count");
    if (isNaN(count) || count === null)
        return 0;

    return parseInt(count);
}

function storeNote(index, time, text)
{
    var note = JSON.stringify({
        note_text: text,
        note_time: time
    });

    localStorage.setItem("count", getCount() + 1);
    localStorage.setItem(index, note);
}

function loadNotes(index)
{
    var count = getCount();
    var note = localStorage.getItem(index);
    var parsed = JSON.parse(note);

    var out = {
        NOTES_LOAD: 1,
        NOTES_LOAD_INDEX: index,
        NOTES_LOAD_TIME: parsed.note_time,
        NOTES_LOAD_TEXT: parsed.note_text
    };

    appMessageQueue.send(out);
}

function deleteNote(index)
{
    var count = getCount();
    localStorage.setItem(index, null);
    localStorage.removeItem(index);
    decrementIndex(index);
    if (count != 0)
        localStorage.setItem("count", count - 1);
}

Pebble.addEventListener("ready",
    function(e) {
        var out = {
            READY: getCount()
        };
        Pebble.sendAppMessage(out, function(e) {
            console.log("WOHOOO");
        }, function(e) {
            console.log("FUCK!");
        });
    }
);

Pebble.addEventListener("appmessage",
    function(e) {
        if (e.payload.NOTES_LOAD != null)
            loadNotes(e.payload.NOTES_LOAD);
        else if (e.payload.NOTES_DELETE != null)
            deleteNote(e.payload.NOTES_DELETE);
        else {
            storeNote(e.payload.NOTES_STORE_INDEX, e.payload.NOTES_STORE_TIME, e.payload.NOTES_STORE_TEXT);
        }
    }
);

Pebble.addEventListener("showConfiguration", function() {
    var url = "http://pebblenotes.mentallysanemainliners.org/api/notes/push";
    var data = [];

    for (var i = 0; i < getCount(); i++) {
        var note = JSON.parse(localStorage.getItem(i));
        if (note == null)
            continue;

        var time = note.note_time;
        var string = note.note_text;

        var data_str = [i, time, string];
        data.push(data_str);
    }

    data = Base64.encode(JSON.stringify(data));
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        if (xhr.status === 200) {
            var resp = JSON.parse(xhr.response);
            Pebble.openURL(resp.url);
        }
    }
    xhr.open("POST", url);
    xhr.send(data);
});

Pebble.addEventListener("webviewclosed", function(e) {
    var response = JSON.parse(Base64.decode(e.response));
    for (var i = 0; i < response.length; i++) {
        var note = response[i];
        var notestring = JSON.stringify({
            note_text: note[2],
            note_time: note[1]
        });
        // TODO: add support for adding notes too
        if (localStorage.getItem(note[0]) == null)
            localStorage.setItem("count", getCount() + 1);

        localStorage.setItem(note[0], notestring);
        loadNotes(note[0]);
    }
});

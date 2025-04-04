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
function storeNote(index, time, text)
{
    console.log("storing a note!", index, time, text);
    localStorage.setItem(index, JSON.stringify({
        note_text: text,
        note_time: time
    }));
}

function loadNotes(index)
{
    console.log("loadNotes called!", index);
    var note = localStorage.getItem(index);
    console.log("note in json is", note);
    var parsed = JSON.parse(note);
    console.log("Fetched note from local storage", parsed);

    var out = {
        NOTES_LOAD: 1,
        NOTES_LOAD_INDEX: index,
        NOTES_LOAD_TIME: parsed.note_time,
        NOTES_LOAD_TEXT: parsed.note_text
    };

    appMessageQueue.send(out);
}

Pebble.addEventListener("ready",
    function(e) {
        console.log("Hello world! - Sent from your javascript application.");
        var out = {
            READY: 1
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
        else {
            storeNote(e.payload.NOTES_STORE_INDEX, e.payload.NOTES_STORE_TIME, e.payload.NOTES_STORE_TEXT);
        }
    }
);
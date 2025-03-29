function storeNote(index, time, text)
{
    localStorage.setItem(index, JSON.stringify({
        note_text: text,
        note_time: time
    }));
}

function loadNotes(index)
{
    var note = localStorage.getItem(index);
    var parsed = JSON.parse(note);
    console.log("Fetched note from local storage", parsed);
}

Pebble.addEventListener("ready",
    function(e) {
        console.log("Hello world! - Sent from your javascript application.");
        var out = {
            NOTES_LOAD: "hello this was sent from pkjs!"
        };
//        Pebble.sendAppMessage(out, function(e) {
//            console.log("WOHOOO");
//        }, function(e) {
//            console.log("FUCK!");
//        });
    }
);

Pebble.addEventListener("appmessage",
    function(e) {
        if (e.payload.NOTES_LOAD)
            loadNotes(e.payload.NOTES_LOAD);
        else {
            storeNote(e.payload.NOTES_STORE_INDEX, e.payload.NOTES_STORE_TIME, e.payload.NOTES_STORE_TEXT);
        }
    }
);
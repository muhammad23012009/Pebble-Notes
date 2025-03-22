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

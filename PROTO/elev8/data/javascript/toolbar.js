var elm = require('elm');

var EXPAND_BOTH = { x : 1.0, y : 1.0 };
var FILL_BOTH = { x : -1.0, y : -1.0 };

var win = elm.realise(elm.Window({
    title: "test",
    width: 480,
    elements: {
        background: elm.Background({
            weight : EXPAND_BOTH,
            align : FILL_BOTH,
            resize : true,
            red : 255,
            green : 0,
            blue : 0,
        }),
        toolbar: elm.Toolbar({
            weight : EXPAND_BOTH,
            align : FILL_BOTH,
            resize : true,
            elements :
            [
                {
                    label: 'Label',
                    on_select: function() {
                        print("Label only clicked")
                    }
                },
                {
                    icon: 'apps',
                    on_select: function() {
                        print("Icon only clicked")
                    }
                }
            ]
        })
    }
}));

function toolbar_cb(item) {

    print('Click: ', item);

    if (item == "CLOSE")
        elm.exit();
}

win.elements.toolbar.append('home', 'Home', 'HOME', toolbar_cb);
win.elements.toolbar.append('chat', 'Chat', 'CHAT', toolbar_cb);

win.elements.toolbar.elements.clock = {
    label : 'Clock',
    icon : 'clock',
    data : 'CLOCK',
    on_select : toolbar_cb
};

win.elements.toolbar.elements.close = {
    label : 'Close',
    icon : 'close',
    data : 'CLOSE',
    on_select : toolbar_cb
};
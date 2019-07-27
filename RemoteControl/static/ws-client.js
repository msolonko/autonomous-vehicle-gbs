$(document).ready(function(){

        var WEBSOCKET_ROUTE = "/ws";

        if(window.location.protocol == "http:"){
            //localhost ssh tunnel
            var ws = new WebSocket("ws://" + window.location.host + WEBSOCKET_ROUTE);
            }
        else if(window.location.protocol == "https:"){
            var ws = new WebSocket("wss://" + window.location.host + WEBSOCKET_ROUTE);
            }

        ws.onopen = function(evt) {
            $("#ws-status").html("Connected");
            $("#ws-status").css('background', 'lightgreen');
            };

        ws.onmessage = function(evt) {
            };

        ws.onclose = function(evt) {
            $("#ws-status").html("Disconnected");
            $("#ws-status").css('background', 'red');
            };
        
         var w,a,s,d,b,l,space,platooning = false;
        
        $('#platoonBtn').click(function() {
            if (platooning) {
                platooning = false;
                ws.send('!platoon');
                $("#platoonBtn").html('platoon');
            } else {
                platooning=true;
                ws.send('platoon');
                $("#platoonBtn").html('stop platooning');
            }
        });
        
        $('#ultrasonicTaskBtn').click(function() {
            ws.send('ultrasonicTask');
        });
        
        $('#parallelParkBtn').click(function() {
            ws.send('parallel');
        });
        
        $('#alphaTurn').click(function() {
            ws.send('alphaTurn');
        });
        
        $('#alphaPark').click(function() {
            ws.send('alphaPark');
        });
        
        $('#garage').click(function() {
            ws.send('garage');
        });
        
        var w,a,s,d,b,l,space = false;
        
        $(document).keydown(function(e) {
             if(e.which == 87) {
                if (!w) {
                    w = true;
                    ws.send('w1');
                }
             }
             if(e.which == 65) {
                if (!a) {
                    a = true;
                    ws.send('a1');
                }
             }
             if(e.which == 83) {
                if (!s) {
                    s = true;
                    ws.send('s1');
                }
             }
             if(e.which == 68) {
                if (!d) {
                    d = true;
                    ws.send('d1');
                }
             }
             if(e.which == 66) {
                if (!b) {
                    b = true;
                    ws.send('b');
                }
             }
             if(e.which == 32) {
                if (!space) {
                    space = true;
                    ws.send('_');
                }
             }
             if(e.which == 76) {
                if (!l) {
                    l = true;
                    ws.send('l');
                }
             }
        });
        
        
        $(document).keyup(function(e) {
             if(e.which == 87) {
                if (w) {
                    w = false;
                    ws.send('w0');
                }
             }
             if(e.which == 65) {
                if (a) {
                    a = false;
                    ws.send('a0');
                }
             }
             if(e.which == 83) {
                if (s) {
                    s = false;
                    ws.send('s0');
                }
             }
             if(e.which == 68) {
                if (d) {
                    d = false;
                    ws.send('d0');
                }
             }
             if(e.which == 66) {
                if (b) {
                    b = false;
                }
             }
             if(e.which == 32) {
                if (space) {
                    space = false;
                    ws.send('_0');
                }
             }
             
             if(e.which == 76) {
                if (l) {
                    l = false;
                }
             }
        });

      });

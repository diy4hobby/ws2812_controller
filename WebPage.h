static const char PROGMEM _WEB_PAGE[] = R"rawliteral(
<html>
    <head>
    		<meta charset="utf-8">
        <style>
            .mainDiv{
                width: 500px;
                margin: auto;
            }
            .parameter{
                display: grid;
                grid-template-columns: 50% 50%;
                margin: 10px;
            }
            .userBtn{
                height: 40px;
                margin: 10px;
            }
            .userButtons{
                display: grid;
                grid-template-columns: calc(100%/3) calc(100%/3) calc(100%/3);
            }
        </style>
    </head>
    <body>
        <div class="mainDiv">

            <div id="headLabel">
                <div style="text-align: center;">LED strip driver</div>
                <div style="text-align: center;">(ESP8266, WS2812b)</div>
            </div>

            <div id="parameters">
                <div id='networkParams'>
                    <fieldset>
                        <legend>Network parameters:</legend>

                        <div class="parameter">
                            <div>
                                Static IP
                            </div>
                            <div>
                                <input type="text" id="network_ip">
                            </div>
                        </div>

                        <div class="parameter">
                            <div>
                                Alias
                            </div>
                            <div>
                                <input type="text" id="network_alias">
                            </div>
                        </div>
                    
                    </fieldset>
                </div>

                <div id='ledParams'>
                    <fieldset>
                        <legend>LED strip parameters:</legend>

                        <div class="parameter">
                            <div>
                                Number of LEDs
                            </div>
                            <div>
                                <input type="number" id="leds_count">
                            </div>
                        </div>

                        <div class="parameter">
                            <div>
                                Effects speed
                            </div>
                            <div>
                                <input type="number" id="leds_speed">
                            </div>
                        </div>

                        <div class="parameter">
                            <div>
                                Pin index
                            </div>
                            <div>
                                <input type="number" id="leds_pin">
                            </div>
                        </div>

                    </fieldset>
                </div>

                <div id='motionSensorParams'>
                    <fieldset>
                        <legend>PIR sensor parameters:</legend>

                        <div class="parameter">
                            <div>
                                Pin index
                            </div>
                            <div>
                                <input type="number" id="pir_pin">
                            </div>
                        </div>

              

                    </fieldset>
                </div>

                <div id='mqttParams'>
                    <fieldset>
                        <legend>MQTT parameters:</legend>

                        <div class="parameter">
                            <div>
                                Server IP
                            </div>
                            <div>
                                <input type="text" id="mqtt_ip">
                            </div>
                        </div>

                        <div class="parameter">
                            <div>
                                Server port
                            </div>
                            <div>
                                <input type="number" id="mqtt_port">
                            </div>
                        </div>

                        <div class="parameter">
                            <div>
                                Username
                            </div>
                            <div>
                                <input type="text" id="mqtt_user">
                            </div>
                        </div>

                        <div class="parameter">
                            <div>
                                Password
                            </div>
                            <div>
                                <input type="text" id="mqtt_pwd">
                            </div>
                        </div>

                    </fieldset>
                </div>

            </div>

            <div id="buttons" class="userButtons">
                <button class="userBtn" onclick="setParameters()">Save</button>
                <button class="userBtn" onclick="enableLED()">LEDs ON</button>
                <button class="userBtn" onclick="disableLED()">LEDs OFF</button>
                <button class="userBtn" onclick="restart()">Restart</button>
            </div>
        </div>
    </body>

    <script>
        var network = {};
        var leds = {};
        var pir = {};
        var mqtt = {};

        // Значения параметров
        network.ip = "{{NETWORK.IP}}";
        network.alias = "{{NETWORK.ALIAS}}";

        leds.count = {{LEDS.COUNT}};
        leds.speed = {{LEDS.SPEED}};
        leds.pin = {{LEDS.PIN}};

        pir.pin = {{PIR.PIN}};

        mqtt.ip = "{{MQTT.IP}}";
        mqtt.port = {{MQTT.PORT}};
        mqtt.user = "{{MQTT.USER}}";
        mqtt.pwd = "{{MQTT.PWD}}"

        // Вывод значений в поля после загрузки
        window.onload = function(){
            document.getElementById("network_ip").value = network.ip;
            document.getElementById("network_alias").value = network.alias;
            
            document.getElementById("leds_count").value = leds.count ;
            document.getElementById("leds_speed").value = leds.speed;
            document.getElementById("leds_pin").value = leds.pin;

            document.getElementById("pir_pin").value = pir.pin;

            document.getElementById("mqtt_ip").value =  mqtt.ip;
            document.getElementById("mqtt_port").value = mqtt.port;
            document.getElementById("mqtt_user").value =  mqtt.user;
            document.getElementById("mqtt_pwd").value = mqtt.pwd;
        }

        // Установка параметров
        function setParameters(){
            //Собираем значения с полей
            network.ip = document.getElementById("network_ip").value;
            network.alias = document.getElementById("network_alias").value;

            leds.count = document.getElementById("leds_count").value;
            leds.speed = document.getElementById("leds_speed").value;
            leds.pin = document.getElementById("leds_pin").value;

            pir.pin = document.getElementById("pir_pin").value;

            mqtt.ip = document.getElementById("mqtt_ip").value;
            mqtt.port = document.getElementById("mqtt_port").value;
            mqtt.user = document.getElementById("mqtt_user").value;
            mqtt.pwd = document.getElementById("mqtt_pwd").value;

            var xhr = new XMLHttpRequest(); 
            var url = "/config?network.ip=" + network.ip + "&" +
                                "network.alias="  + network.alias + "&" +
                                "leds.count="  + leds.count + "&" +
                                "leds.speed="  + leds.speed + "&" +
                                "leds.pin="  + leds.pin + "&" +
                                "pir.pin="  + pir.pin + "&" +
                                "mqtt.ip="  + mqtt.ip + "&" +
                                "mqtt.port="  + mqtt.port + "&" +
                                "mqtt.user="  + mqtt.user + "&" +
                                "mqtt.pwd="  + mqtt.pwd;
            xhr.open("GET", url, true);
            
            xhr.timeout = 5000;
            //xhr.responseType = "json";
            
            xhr.onreadystatechange = (function(){
                if (xhr.readyState == 4 && xhr.status == 200 ){
                    if (xhr.response != null){
                        
                    }
                }
            });
            xhr.ontimeout = function(){};
            xhr.onerror = function(){};
            xhr.send();    
        }

        // Включение светодиодной ленты
        function enableLED(){
            let xhr = new XMLHttpRequest(); 
            let url = "/leds_on";
            xhr.open("GET", url, true);
            
            xhr.timeout = 5000;
            //xhr.responseType = "json";
            
            xhr.onreadystatechange = (function(){
                if (xhr.readyState == 4 && xhr.status == 200 ){
                    if (xhr.response != null){
                        
                    }
                }
            });
            xhr.ontimeout = function(){};
            xhr.onerror = function(){};
            xhr.send();  
        }

        // Отключение светодиодной ленты
        function disableLED(){
            let xhr = new XMLHttpRequest(); 
            let url = "/leds_off";
            xhr.open("GET", url, true);
            
            xhr.timeout = 5000;
            //xhr.responseType = "json";
            
            xhr.onreadystatechange = (function(){
                if (xhr.readyState == 4 && xhr.status == 200 ){
                    if (xhr.response != null){
                        
                    }
                }
            });
            xhr.ontimeout = function(){};
            xhr.onerror = function(){};
            xhr.send();
        }

        // Перезагрузка модуля
        function restart(){
            let xhr = new XMLHttpRequest(); 
            let url = "/restart";
            xhr.open("GET", url, true);
            
            xhr.timeout = 5000;
            //xhr.responseType = "json";
            
            xhr.onreadystatechange = (function(){
                if (xhr.readyState == 4 && xhr.status == 200 ){
                    if (xhr.response != null){
                        
                    }
                }
            });
            xhr.ontimeout = function(){};
            xhr.onerror = function(){};
            xhr.send();
        }


    </script>
</html>
)rawliteral";

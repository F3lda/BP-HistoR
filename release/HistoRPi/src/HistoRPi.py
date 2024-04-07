"""
 * @file HistoRPi.py
 *
 * @brief HistoRPi - Audio streaming device for historic radio receivers
 * @date 2024-01-28
 * @author F3lda (Karel Jirgl)
 * @update 2024-04-07 (v1.0)
"""
from flask import Flask,request,redirect,url_for
import subprocess
from threading import Thread
import os
import time
from pathlib import Path
import urllib.parse
from shutil import rmtree
import tempfile
from multiprocessing import Process
# user repr() as var_dump()



app = Flask(__name__)

wifi_device = "wlan0"
conf_file = "device.conf"
web_file = "web.conf"





@app.route('/')
def index():
    ## Disable IPtoSPEECH
    raspi_disablevoiceip()

    ## read config file
    web_config = read_web_config()





    webpageui = "<head><title>HistoRPi - Home Page</title></head><h1>HistoRPi - audio streaming device for historic radios</h1><hr>"



    #
    # AUDIOOUTPUTS
    ##########################
    webpageui += """
    <h2>AudioOutputs</h2>
    <form id="AudioOutputs" action="/audiooutputs" method="POST">
    <table border="1">
        <tr>
            <th colspan="5">DEVICE</th>
            <th>SOURCE</th>
            <th colspan="3">CONTROLS</th>
        </tr>
        <tr>
            <td>DEFAULT</td>
            <td>ID</td>
            <td>NAME</td>
            <td>STATE</td>
            <td>VOLUME</td>
            <td>AUDIO SOURCE</td>
            <td>PLAYING</td>
            <td>AUTOPLAY</td>
            <td>CONTROLS</td>
        </tr>"""


    default_sink = audio_get_default_sink()
    device_sinks = audio_get_sinks(default_sink)

    for sink in device_sinks:

        os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
        os.chdir("audio_config")

        sink_config = {"AU_sink": sink["uuid"], "AU_volume": sink["volume"], "AU_source": 'SD', "AU_playing": '0', "AU_autoplay": '0', "AU_controls-SD-track": 'No track', "AU_controls-SD-repeat": '1', "AU_controls-SD-shuffle": '1', "AU_controls-URL-url": '', "AU_controls-FM-freq": '', "AU_controls-BT-name": '', "AU_controls-DAB-channel": '', "AU_controls-DAB-station": ''}

        with open(sink["uuid"]+'.conf', "a+") as file:
            pass
        with open(sink["uuid"]+'.conf', "r+") as file:
            for line in file:
                line = line.strip()
                item = line.split('=',1)
                if len(item) == 2:
                    sink_config[item[0]] = item[1]


        webpageui += f"""
  		<tr><input type="hidden" name="AU_sink[{sink["id"]}]" value="{sink["uuid"]}">
            <td><input type="radio" name="AU_default" value="{sink["uuid"]}" {sink["default"]}></td>
            <td>{sink["id"]}</td>
            <td>{sink["name"]}</td>
            <td>{sink["state"]}</td>
            <td><input type="number" name="AU_volume[{sink["id"]}]" min="1" max="120" value="{sink["volume"]}" size="5">%</td>
            <td>
                <input type="hidden" name="AU_source[{sink["id"]}]" value="{sink_config["AU_source"]}">
                <select class="cls-controls" onchange="change_controls({sink["id"]}, this.value); this.previousElementSibling.value=this.value;" {'disabled' if process_sink_playing(sink["uuid"]) != "" else ''}>
                    <option value="SD" {'selected' if sink_config["AU_source"] == "SD" else ''}>SDcard player</option>
                    <option value="URL" {'selected' if sink_config["AU_source"] == "URL" else ''}>URL player</option>
                    <option value="FM" {'selected' if sink_config["AU_source"] == "FM" else ''}>FM radio</option>
                    <option value="BT" {'selected' if sink_config["AU_source"] == "BT" else ''}>Bluetooth</option>
                    <option value="DAB" {'selected' if sink_config["AU_source"] == "DAB" else ''}>DAB radio</option>
                </select>
            </td>
            <td><input type="checkbox" {'checked' if process_sink_playing(sink["uuid"]) != "" else ''} disabled></td>
            <td><input type="hidden" name="AU_autoplay[{sink["id"]}]" value="{sink_config["AU_autoplay"]}"><input type="checkbox" onclick="this.previousElementSibling.value=1-this.previousElementSibling.value" {'checked' if sink_config["AU_autoplay"] == "1" else ''}></td>
            <td id="controls-{sink["id"]}">

                <span class="controls-{sink["id"]}-SD">
                    <input type="hidden" name="AU_controls-SD-track[{sink["id"]}]" value="{sink_config["AU_controls-SD-track"]}">
                    <input type="text" value="{process_sink_get_track(sink["uuid"]) if process_sink_get_track(sink["uuid"]) != "" else sink_config["AU_controls-SD-track"]}" disabled> -
                    <input type="submit" name="SD-previous" value="<" title="previous">
                    <input type="submit" name="SD-pause-resume" value="|>" title="pause/resume">
                    <input type="submit" name="SD-stop" value="O" title="stop">
                    <input type="submit" name="SD-next" value=">" title="next"> -
                    repeat: <input type="hidden" name="AU_controls-SD-repeat[{sink["id"]}]" value="{sink_config["AU_controls-SD-repeat"]}"><input type="checkbox" onclick="this.previousElementSibling.value=1-this.previousElementSibling.value" {'checked' if sink_config["AU_controls-SD-repeat"] == "1" else ''}>
                    <span style="cursor:help; text-decoration:underline; text-decoration-style: dotted;" title="Random play is checked only when SDcard player starts playing!">shuffle</span>: <input type="hidden" name="AU_controls-SD-shuffle[{sink["id"]}]" value="{sink_config["AU_controls-SD-shuffle"]}"><input type="checkbox" onclick="this.previousElementSibling.value=1-this.previousElementSibling.value" {'checked' if sink_config["AU_controls-SD-shuffle"] == "1" else ''}>
                    - <a class="trackselect" data-sink="{sink["uuid"]}" href="/sdcard?sink={urllib.parse.quote_plus(sink["uuid"])}">select track</a>
                </span>

                <span class="controls-{sink["id"]}-URL" style="display:none">
                    <input type="text" name="AU_controls-URL-url[{sink["id"]}]" value="{sink_config["AU_controls-URL-url"]}" placeholder="URL" title="URL">
                    <input type="submit" name="URL-play" value="|>" title="play">
                    <input type="submit" name="URL-stop" value="O" title="stop">
                </span>

                <span class="controls-{sink["id"]}-FM" style="display:none">
                    <input type="text" name="AU_controls-FM-freq[{sink["id"]}]" value="{sink_config["AU_controls-FM-freq"]}" placeholder="FREQ" title="FREQ">
                    <input type="submit" name="FM-play" value="|>" title="play">
                    <input type="submit" name="FM-stop" value="O" title="stop">
                </span>

                <span class="controls-{sink["id"]}-BT" style="display:none">
                    <!---<input type="text" name="AU_controls-BT-name[{sink["id"]}]" value="{sink_config["AU_controls-BT-name"]}" placeholder="BT NAME" title="BT NAME">--->
                    <input type="submit" name="BT-start" value="ON" title="play">
                    <input type="submit" name="BT-stop" value="OFF" title="stop">
                </span>

                <span class="controls-{sink["id"]}-DAB" style="display:none">
                    <input type="text" name="AU_controls-DAB-channel[{sink["id"]}]" value="{sink_config["AU_controls-DAB-channel"]}" placeholder="CHANNEL" title="CHANNEL">
                    <!---<input type="text" name="AU_controls-DAB-station[{sink["id"]}]" value="{sink_config["AU_controls-DAB-station"]}" placeholder="STATION" title="STATION">--->
                    <input type="submit" name="DAB-play" value="|>" title="play">
                    <input type="submit" name="DAB-stop" value="O" title="stop">
                    <input type="submit" name="DAB-tuneup" value="/\\" title="tune up">
                    <input type="submit" name="DAB-tunedown" value="\/" title="tune down">
                </span>

            </td>
        </tr>
    """

    ###<input type="hidden" name="TS_autoplay" value="{web_config["TS_autoplay"]}"><input type="checkbox" onclick="this.previousElementSibling.value=1-this.previousElementSibling.value" {'checked' if web_config["TS_autoplay"] == "1" else ''}></td>
    ###<input type="hidden" name="TS_autoplay" value="{web_config["TS_autoplay"]}"><input type="checkbox" onclick="this.previousElementSibling.value=1-this.previousElementSibling.value" {'checked' if web_config["TS_autoplay"] == "1" else ''}></td>

    webpageui += """
    </table>
    </form>
    """





    #
    # TRANSMITTERS
    ##########################
    webpageui += f"""
    <h2>Transmitters</h2>
    <pre>!!! WARNING !!! - RaspberryPi's WiFi connection is interfered with AM transmission -> use connection over Ethernet cable !!! (Also nearby other transmitting devices can cause interference!)</pre>
    <form id="Transmitters" action="/transmitters" method="POST">
    <table border="1">
        <tr>
            <th>LIVE</th>
            <th>TRANS</th>
            <th>FREQ</th>
            <th>SOURCE</th>
            <th style="cursor:help; text-decoration:underline; text-decoration-style: dotted;"
                title="If checked, automatically put ON AIR after boot-up">AUTOPLAY</th>
        </tr>
        <tr>
            <td rowspan="2">
                <div id="ck-button"><label>
                    <input type="hidden" name="TS_live" value="{'1' if (os.system("ps cax | grep pifmrds") == 0 or os.system("ps cax | grep rpitx") == 0) else '0'}"><input type="checkbox" onclick="this.previousElementSibling.value=1-this.previousElementSibling.value" {'checked' if (os.system("ps cax | grep pifmrds") == 0 or os.system("ps cax | grep rpitx") == 0) else ''}><span>ON AIR</span>
                </label></div>
            </td>
            <td>
                <input type="radio" name="TS_trans" value="FM" {'checked' if web_config["TS_trans"] == "FM" else ''}>
                FM - <input type="text" name="TS_desc-8ch" value="{web_config["TS_desc-8ch"]}" size="8" maxlength="8">
                - <input type="text" name="TS_desc-long" value="{web_config["TS_desc-long"]}">
            </td>
            <td rowspan="2"><input type="number" name="TS_freq" min="1" max="120" value="{web_config["TS_freq"]}" size="8" /> MHz</td>
            <td rowspan="2" style="text-align: center;">
                <select name="TS_source">
                    {''.join(["<option value='"+sink["uuid"]+"' selected>["+sink["id"]+"] "+sink["name"]+"</option>" if web_config["TS_source"] == sink["uuid"] else "<option value='"+sink["uuid"]+"'>["+sink["id"]+"] "+sink["name"]+"</option>" for sink in device_sinks])}
                </select>
            </td>
            <td rowspan="2" style="text-align: center;">
                <input type="hidden" name="TS_autoplay" value="{web_config["TS_autoplay"]}"><input type="checkbox" onclick="this.previousElementSibling.value=1-this.previousElementSibling.value" {'checked' if web_config["TS_autoplay"] == "1" else ''}>
            </td>
        </tr>
        <tr>
            <td><input type="radio" name="TS_trans" value="AM" {'checked' if web_config["TS_trans"] == "AM" else ''}> AM</td>
        </tr>
    </table>
    </form>
    <pre>WARNING: FM radio, DAB radio and Bluetooth are not working while transmitting from RaspberryPi using SDR!!! (there is probably interference)</pre>
    <hr>
    """





    #
    # SETTINGS
    ##########################
    webpageui += """
    <h2>Settings</h2>
    <h3>Central STOPS</h3>
    Stop all Audio players: <a href="./AUstop">STOP AUDIO PLAYERS</a><br><br>
    Stop all Transmitters: <a href="./TRstop">STOP TRANSMITTERS</a><br>
    <h3>Network state</h3>
    IP addresses:
    """
    try:
        result = subprocess.check_output("hostname -I", shell=True)
        webpageui += "<strong>"+str(', '.join(result.decode().strip().split(' ')))+"</strong>"
    except subprocess.CalledProcessError as e:
        webpageui += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)

    webpageui += "<br>Devices:<br>"
    try:
        result = subprocess.check_output("nmcli --colors no -m multiline connection show --active", shell=True)
        cells_list = result.decode().strip().split("\n")
        connections_list = list_join_span(cells_list, "\n", 4)

        webpageui += "<table border='1'><tr><th>DEVICE</th><th>TYPE</th><th>UUID</th><th>NAME</th></tr>"
        for device in connections_list:
            webpageui += "<tr>"
            device_cells = device.split('\n')
            device_cells.reverse()
            if len(device_cells) == 4:
                for device_cell in device_cells:
                    webpageui += "<td>"+device_cell.split(":",1)[1].strip()+ "</td>"
            webpageui += "</tr>"
        webpageui += "</table>"
    except subprocess.CalledProcessError as e:
        webpageui += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)



    webpageui += """
    <h3>WiFi saved connections</h3>
    """
    try:
        result = subprocess.check_output("nmcli --colors no -m multiline connection show", shell=True)
        cells_list = result.decode().strip().split("\n")
        connections_list = list_join_span(cells_list, "\n", 4)

        webpageui += "<table border='1'><tr><th>NAME</th><th>UUID</th><th>TYPE</th><th>DEVICE</th><th>X</th></tr>"
        for connection in connections_list:
            webpageui += "<tr>"
            connection_cells = connection.split('\n')
            if len(connection_cells) == 4 and connection_cells[0].replace(" ", "") != "NAME:Hotspot" and connection_cells[2].replace(" ", "") == "TYPE:wifi":
                for connection_cell in connection_cells:
                    webpageui += "<td>"+connection_cell.split(":",1)[1].strip()+ "</td>"
                webpageui += "<td><a href='/removewifi/"+connection_cells[1].split(":",1)[1].strip()+"/ssid/"+connection_cells[0].split(":",1)[1].strip()+"'>delete connection</a></td>"
            webpageui += "</tr>"
        webpageui += "</table>"

    except subprocess.CalledProcessError as e:
        webpageui += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)

    try:
        result = subprocess.check_output("nmcli --colors no device wifi show-password | grep 'SSID:' | cut -d ':' -f 2", shell=True)
        webpageui += """
        <br>Connected to WiFi: <strong>"""+str(result.decode().strip())+"""</strong><br>
        """
    except subprocess.CalledProcessError as e:
        webpageui += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)

    webpageui += """
    <a href="./disconnect">Delete current WiFi connection & Reboot</a>
    """



    saved_SSID = ""
    saved_PASS = ""
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
    with open(conf_file, "a+") as file:
        pass
    with open(conf_file, "r+") as file:
        for line in file:
            line = line.strip()
            if line.startswith("WIFI_SSID"):
                saved_SSID = line.removeprefix("WIFI_SSID=\"").removesuffix("\"")
            elif line.startswith("WIFI_PASSWORD"):
                saved_PASS = line.removeprefix("WIFI_PASSWORD=\"").removesuffix("\"")

    webpageui += """
    <h3>WiFi to connect on next boot-up</h3>
    <form id="SaveWifi" action="/savewifi" method="post">
        <label for="ssid">SSID: <input type="text" name="ssid" value=\""""+ saved_SSID +""""/></label>
        <label for="password">Password: <input type="text" name="password" value=\""""+ saved_PASS +""""/></label>
        <input type="submit" value="Save">
        <p></p>
    </form>



    <h3>IPtoSpeech</h3>
    <a href="./disablevoiceip">Disable IPtoSpeech</a>
    <pre>Note: IPtoSpeech is automatically disabled when this page is loaded after boot-up</pre>



    <h3>System</h3>
    <a href="./reboot">Reboot</a><br>
    <a href="./shutdown">Shutdown</a>


    <script>
        // Disabling form submit by enter key
        // Source: https://stackoverflow.com/a/37241980
        window.addEventListener('keydown',function(e) {
            if (e.keyIdentifier=='U+000A' || e.keyIdentifier=='Enter' || e.keyCode==13) {
                if (e.target.type != 'textarea' && e.target.type != 'submit' && e.target.type != 'button') {
                    e.preventDefault();
                    return false;
                }
            }
        }, true);


        // automatically change audio source controls onload
        window.addEventListener("load", function(event) {
            var list = document.getElementsByClassName("cls-controls") ;
            for (let item of list) {
                item.dispatchEvent(new Event('change'));
            }
        });

        function change_controls(id, value)
        {
            var controls = document.getElementById("controls-"+id)

            for(i = 0; i < controls.children.length; i++) {
                if (controls.children[i].className == "controls-"+id+"-"+value) {
                    controls.children[i].style.display = '';
                } else {
                    controls.children[i].style.display = 'none';
                }
            }
        }



        document.body.addEventListener("change", function(e) {
            if (e.target.form != null && e.target.form.id != null) {
                if (e.target.form.id == "Transmitters" || e.target.form.id == "AudioOutputs") {
                    httpPOST(e.target.form.action, new FormData(e.target.form));
                }
            }
        });

        document.body.addEventListener("submit", function(e) {

            if (e.target != null && e.target.id != null) {
                if (e.target.id == "SaveWifi") {
                    httpPOST(e.target.action, new FormData(e.target));
                } else if (e.target.id == "AudioOutputs") {
                    formData = new FormData(e.target);
                    formData.append('sink_id', e.submitter.closest('tr').firstElementChild.name.split("[")[1].split("]")[0]);
                    formData.append('sink_uuid', e.submitter.closest('tr').firstElementChild.value);
                    formData.append('source', e.submitter.name.split("-")[0]);
                    formData.append('button', e.submitter.name);
                    httpPOST(e.target.action+"-button", formData);
                }
                e.preventDefault();
            }
        });

        document.body.addEventListener("click", function(e) {
            if (e.target && e.target.nodeName == "A") {
                if (!e.target.classList.contains('trackselect')) {
                    httpGET(e.target.href);
                    e.preventDefault();
                } else {
                    //e.target.dataset["sink"]

                    e.target.href += "&options="
                    url = ""
                    if (e.target.previousElementSibling.previousElementSibling.previousElementSibling.previousElementSibling.checked) {
                        url += "-loop 0 ";
                    }
                    if (e.target.previousElementSibling.checked) {
                        url += "-shuffle ";
                    }
                    e.target.href += encodeURIComponent(url);
                }
            }
        });

        async function httpGET(url) {
            // only one http request at a time
            if (document.body.style.cursor == 'wait') {
                return;
            }

            document.body.style.cursor = 'wait';
            try {
                const response = await fetch(url);
                const result = await response.text();

                console.log("Success: " + result);
                if (result != "") {
                    alert(result);
                }
            } catch (error) {
                console.error("Error: " + error + '\\nIf rebooting or shutting down: Success!');
                alert("Error: " + error + '\\nIf rebooting or shutting down: Success!');
            }
            document.body.style.cursor = 'auto';
        }

        async function httpPOST(action, data) {
            // only one http request at a time
            if (document.body.style.cursor == 'wait') {
                return;
            }

            document.body.style.cursor = 'wait';
            try {
                const response = await fetch(action, {
                    method: "POST",
                    body: data
                });
                const result = await response.text();

                console.log("Success: " + result);
                if (result != "") {
                    const isError = result.toLowerCase().includes('error');
                    if (!isError) {
                        var forms = document.forms;
                        for (var i = 0; i < forms.length; i++) {
                            var elements = forms[i];
                            for (var j = 0; j < elements.length; j++) {
                                elements[j].disabled = true;
                            }
                        }
                    }
                    alert(result);
                    if (!isError) {
                        window.location.href = window.location.href.split('#')[0];
                        // if result is not empty disable all forms elements and refresh page OR maybe return result and check it outside the function
                    }
                }
            } catch (error) {
                console.error("Error: " + error);
                alert("Error: " + error);
            }
            document.body.style.cursor = 'auto';
        }

    </script>
    <style>
        /* source: https://jsfiddle.net/zAFND/4/   */
        div label input {
           margin-right:100px;
        }
        body {
            font-family:sans-serif;
        }

        #ck-button {
            margin:4px;
            background-color:#EFEFEF;
            border-radius:4px;
            border:1px solid #D0D0D0;
            overflow:auto;
            float:left;
        }

        #ck-button:hover {
            background:red;
        }

        #ck-button label {
            float:left;
            width:4.0em;
        }

        #ck-button label span {
            text-align:center;
            padding:3px 0px;
            display:block;
        }

        #ck-button label input {
            position:absolute;
            top:-20px;
        }

        #ck-button input:checked + span {
            background-color:#911;
            color:#fff;
        }
    </style>
    """
    return webpageui





##########################
# COMMON FUNCTIONS
##########################
def list_join_span(array, separator, span):
    # https://stackoverflow.com/questions/1621906/is-there-a-way-to-split-a-string-by-every-nth-separator-in-python
    return [separator.join(array[i:i+span]) for i in range(0, len(array), span)]


def config_file_change_value(path, key, value):
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory

    value_changed = False
    new_content = ""
    with open(path, "a+") as file:
        pass
    with open(path, "r+") as file:
        for line in file:
            line = line.strip()
            if line.startswith(key):
                if line != key+'='+value:
                    new_content += key+'='+value+'\n'
                    value_changed = True
            else:
                new_content += line+'\n'
        if value_changed:
            file.seek(0,0)
            file.write(new_content)
            file.truncate()
    return value_changed


def config_file_get_value(path, key):
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory

    value = ''
    with open(path, "a+") as file:
        pass
    with open(path, "r+") as file:
        for line in file:
            line = line.strip()
            if line.startswith(key):
                val = line.split('=',1)
                if len(val) == 2:
                    value = val[1]
    return value


def read_web_config():
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory

    settings_vars = ["TS_live", "TS_trans", "TS_desc-8ch", "TS_desc-long", "TS_freq", "TS_source", "TS_autoplay", "AU_default"]

    file_items = {key: '' for key in settings_vars}
    with open(web_file, "a+") as file:
        pass
    with open(web_file, "r+") as file:
        for line in file:
            line = line.strip()
            item = line.split('=',1)
            if len(item) == 2:
                file_items[item[0]] = item[1]
    return file_items


def audio_get_sinks(default_sink):
    try:
        device_sinks = []

        result = subprocess.check_output("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list sinks | grep --color=never 'Sink \|^[[:space:]]Volume: \|^[[:space:]]Description: \|^[[:space:]]Name: \|^[[:space:]]State: '", shell=True)
        result = result.decode().strip()
        result = result.split('\n')
        curr_sink = 0

        sink_vars = ["id", "uuid", "default", "name", "state", "volume"]
        device_sink = {key: '' for key in sink_vars}
        for i, line in enumerate(result):
            if line.startswith('Sink #'):
                if curr_sink != int(line.removeprefix('Sink #')):
                    device_sinks.append(device_sink.copy())
                    device_sink.clear()
                    device_sink = {key: '' for key in sink_vars}
                    curr_sink += 1
                device_sink["id"] = line.removeprefix('Sink #')
            else:
                line = line.strip()
                if line.startswith('State: '):
                    line = line.removeprefix('State: ')
                    device_sink["state"] = line
                elif line.startswith('Name: '):
                    line = line.removeprefix('Name: ')
                    if line == default_sink:
                        device_sink["default"] = 'checked'
                    device_sink["uuid"] = line
                elif line.startswith('Description: '):
                    line = line.removeprefix('Description: ')
                    device_sink["name"] = line
                elif line.startswith('Volume: '):
                    line = line.split('/')
                    if len(line) > 1:
                        line = line[1].strip().removesuffix('%')
                    else:
                        line = '(unknown)'
                    device_sink["volume"] = line
        device_sinks.append(device_sink.copy())

        return device_sinks

    except subprocess.CalledProcessError as e:
        print("<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output))
    return []


def audio_get_default_sink():
    try:
        result = subprocess.check_output("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl info | grep --color=never 'Default Sink: '", shell=True)
        default_sink = result.decode().strip().removeprefix("Default Sink: ")
        return default_sink
    except subprocess.CalledProcessError as e:
        print("<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output))
    return ''


def check_autoplays():
    # Transmitters check autoplay
    if config_file_get_value(web_file, "TS_autoplay") == '1':
        transmitter = config_file_get_value(web_file, "TS_trans")
        if transmitter == "FM":
            raspi_transFM(config_file_get_value(web_file, "TS_source"), config_file_get_value(web_file, "TS_freq"), config_file_get_value(web_file, "TS_desc-8ch"), config_file_get_value(web_file, "TS_desc-long"))
        elif transmitter == "AM":
            raspi_transAM(config_file_get_value(web_file, "TS_source"), config_file_get_value(web_file, "TS_freq"))

    # check Audio Outputs autoplays
    default_sink = audio_get_default_sink()
    device_sinks = audio_get_sinks(default_sink)
    print("\n\ncheck_autoplays\n\n")
    started_audio_sources = ""
    for sink in device_sinks:
        os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
        os.chdir("audio_config")

        sink_config = {"AU_sink": sink["uuid"], "AU_volume": sink["volume"], "AU_source": 'SD', "AU_playing": '0', "AU_autoplay": '0', "AU_controls-SD-track": 'No track', "AU_controls-SD-repeat": '1', "AU_controls-SD-shuffle": '1', "AU_controls-URL-url": '', "AU_controls-FM-freq": '', "AU_controls-BT-name": '', "AU_controls-DAB-channel": '', "AU_controls-DAB-station": ''}

        with open(sink["uuid"]+'.conf', "a+") as file:
            pass
        with open(sink["uuid"]+'.conf', "r+") as file:
            for line in file:
                line = line.strip()
                item = line.split('=',1)
                if len(item) == 2:
                    sink_config[item[0]] = item[1]

        if sink_config["AU_autoplay"] == '1':
            if sink_config["AU_source"] not in started_audio_sources: # only one instance of every Audio Source
                started_audio_sources += ';'+sink_config["AU_source"]
                if sink_config["AU_source"] == 'SD':
                    # check file exists and add options
                    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
                    path = sink_config["AU_controls-SD-track"].split("'")
                    if len(path) > 2:
                        if not os.path.exists(path[1]):
                            continue

                    options = ''
                    if sink_config["AU_controls-SD-repeat"] == '1':
                        options += "-loop 0 "
                    if sink_config["AU_controls-SD-shuffle"] == '1':
                        options += "-shuffle "

                    raspi_playSD(sink_config["AU_sink"], sink_config["AU_controls-SD-track"], options)
                elif sink_config["AU_source"] == 'URL':
                    raspi_playURL(sink_config["AU_sink"], sink_config["AU_controls-URL-url"])
                elif sink_config["AU_source"] == 'FM':
                    raspi_playFM(sink_config["AU_sink"], sink_config["AU_controls-FM-freq"])
                elif sink_config["AU_source"] == 'BT':
                    # check if current sink is default
                    if sink_config["AU_sink"] == default_sink:
                        raspi_playBT(sink_config["AU_sink"])
                elif sink_config["AU_source"] == 'DAB':
                    # check if current sink is default
                    if sink_config["AU_sink"] == default_sink:
                        raspi_playDAB(sink_config["AU_sink"], sink_config["AU_controls-DAB-channel"])





##########################
# AUDIOOUTPUTS
##########################
@app.route('/audiooutputs', methods=['POST'])
def raspi_audiooutputs():
    if request.method == 'POST':
        audio_conf = dict(zip(list(request.form.keys()), list(request.form.values())))



        output = ""


        sinks = []
        for item in list(request.form.keys()):
            if item.startswith("AU_sink["):
                sinks.append(item.removeprefix("AU_sink[").removesuffix("]"))


        ## save AudioOutputs data
        for sinkid in sinks:


            output += ' AU_sink['+str(sinkid)+']; '



            os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
            os.chdir("audio_config")

            new_content = ""
            file_changed = False
            with open(audio_conf['AU_sink['+str(sinkid)+']']+'.conf', "a+") as file:
                pass
            with open(audio_conf['AU_sink['+str(sinkid)+']']+'.conf', "r+") as file:
                for item in list(request.form.keys()):
                    if item.endswith('['+str(sinkid)+']'):
                        # on AUDIO SOURCE change -> check if current sink is not playing
                        if item.startswith('AU_source'):
                            sink_playing = process_sink_playing(audio_conf['AU_sink['+str(sinkid)+']'])
                            if sink_playing != '' and sink_playing != audio_conf[item]:
                                return "ERROR: can't change AUDIO SOURCE while it is playing!"


                        # add new line
                        new_content += item.removesuffix('['+str(sinkid)+']')+'='+audio_conf[item]+'\n'

                        # find same line
                        file.seek(0,0)
                        if not item.removesuffix('['+str(sinkid)+']')+'='+audio_conf[item]+'\n' in file.readlines():
                            file_changed = True
                            ## item changed! -> do action

                            # change volume
                            if item.startswith('AU_volume'):
                                os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-sink-volume "+audio_conf['AU_sink['+str(sinkid)+']']+" "+audio_conf['AU_volume['+str(sinkid)+']']+"%")




                if file_changed: # don't override when file has not changed
                    file.seek(0,0)
                    file.write(new_content)
                    file.truncate()



                output += repr(file_changed)



        ## check default sink changed and save to web config
        os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory

        sink_conf_exists = False
        sink_conf_changed = False
        new_content = ""
        with open(web_file, "a+") as file:
            pass
        with open(web_file, "r+") as file:
            file.seek(0,0)
            for line in file:
                line = line.strip()

                if line.startswith("AU_default"):
                    sink_conf_exists = True
                    new_content += "AU_default="+audio_conf['AU_default']+'\n'

                    if line != "AU_default="+audio_conf['AU_default']:
                        # default sink changed
                        sink_conf_changed = True

                else:
                    new_content += line+'\n'


            if not sink_conf_exists:
                new_content += "AU_default="+audio_conf['AU_default']+'\n'
                sink_conf_changed = True


            if sink_conf_changed:
                # change defualt sink
                os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-default-sink "+audio_conf['AU_default'])

                file.seek(0,0)
                file.write(new_content)
                file.truncate()

        return ''
        #return repr(sink_conf_changed)+output + repr(audio_conf)

    return 'ERROR: wrong value!'



# AUDIOOUTPUTS - BUTTONS
##########################
@app.route('/audiooutputs-button', methods=['POST'])
def raspi_audiooutputsbutton():
    if request.method == 'POST':
        audio_conf = dict(zip(list(request.form.keys()), list(request.form.values())))



        # when BLUETOOTH start -> check if the SINK is DEFAULT
        if (audio_conf["button"] == "BT-start" and audio_conf["sink_uuid"] != audio_conf["AU_default"]):
            return "ERROR: Bluetooth can play only on the default SINK!"

        # when DAB player start -> check if the SINK is DEFAULT
        if (audio_conf["button"] == "DAB-play" and audio_conf["sink_uuid"] != audio_conf["AU_default"]):
            return "ERROR: DAB radio can play only on the default SINK!"



        ### Audiosources control buttons

        ### SDcard player
        if (audio_conf["button"] == "SD-pause-resume"):
            # check if the SDcard player is playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
                os.system('echo "pause" > mplayercontrol.pipe')
                return ''
            else:
                return "ERROR: Nothing paused/resumed - SDcard player is not playing or not on this SINK!"
        if (audio_conf["button"] == "SD-previous"):
            # check if the SDcard player is playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
                os.system('echo "pt_step -1" > mplayercontrol.pipe')
                return ''
            else:
                return "ERROR: No previous track - SDcard player is not playing or not on this SINK!"
        if (audio_conf["button"] == "SD-next"):
            # check if the SDcard player is playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
                os.system('echo "pt_step 1" > mplayercontrol.pipe')
                return ''
            else:
                return "ERROR: No next track - SDcard player is not playing or not on this SINK!"

        if (audio_conf["button"] == "SD-stop"):
            # check if the SDcard player is already playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.system("sudo kill -9 "+process_find_lowest(audio_conf["source"], audio_conf["sink_uuid"]))
                return "SDcard player stopped!"
            else:
                return "ERROR: Nothing stopped - SDcard player is not playing or not on this SINK!"



        ### URL player
        if (audio_conf["button"] == "URL-play"):
            # check if the same AUDIO SOURCE is already playing
            sink_uuid = process_source_playing("URL")
            if (sink_uuid != ""):
                return "ERROR: "+audio_conf["source"]+ " audio source is already playing on sink: "+sink_uuid
            else:
                return raspi_playURL(audio_conf["sink_uuid"], audio_conf["AU_controls-URL-url["+audio_conf["sink_id"]+"]"])
        if (audio_conf["button"] == "URL-stop"):
            # check if the URL player is already playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.system("sudo kill -9 "+process_find_lowest(audio_conf["source"], audio_conf["sink_uuid"]))
                return "URL player stopped!"
            else:
                return "ERROR: Nothing stopped - URL player is not playing or not on this SINK!"



        ### FM player
        if (audio_conf["button"] == "FM-play"):
            # check if the same AUDIO SOURCE is already playing
            sink_uuid = process_source_playing("FM")
            if (sink_uuid != ""):
                return "ERROR: "+audio_conf["source"]+ " audio source is already playing on sink: "+sink_uuid
            else:
                return raspi_playFM(audio_conf["sink_uuid"], audio_conf["AU_controls-FM-freq["+audio_conf["sink_id"]+"]"])
        if (audio_conf["button"] == "FM-stop"):
            # check if the FM player is already playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.system("sudo kill -9 "+process_find_lowest(audio_conf["source"], audio_conf["sink_uuid"]))
                return "FM player stopped!"
            else:
                return "ERROR: Nothing stopped - FM player is not playing or not on this SINK!"



        ### Bluetooth
        if (audio_conf["button"] == "BT-start"):
            # check if the same AUDIO SOURCE is already playing
            sink_uuid = process_source_playing("BT")
            if (sink_uuid != ""):
                return "ERROR: "+audio_conf["source"]+ " audio source is already playing on sink: "+sink_uuid
            else:
                return raspi_playBT(audio_conf["sink_uuid"])
        if (audio_conf["button"] == "BT-stop"):
            # check if the BT player is already playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.system("sudo kill -SIGINT "+process_find_lowest(audio_conf["source"], audio_conf["sink_uuid"]))
                return "Bluetooth stopped!"
            else:
                return "ERROR: Nothing stopped - Bluetooth is not playing or not on this SINK!"



        ### DAB player
        if (audio_conf["button"] == "DAB-play"):
            # check if the same AUDIO SOURCE is already playing
            sink_uuid = process_source_playing("DAB")
            if (sink_uuid != ""):
                return "ERROR: "+audio_conf["source"]+ " audio source is already playing on sink: "+sink_uuid
            else:
                return raspi_playDAB(audio_conf["sink_uuid"], audio_conf["AU_controls-DAB-channel["+audio_conf["sink_id"]+"]"]) #, audio_conf["AU_controls-DAB-station["+audio_conf["sink_id"]+"]"]
        if (audio_conf["button"] == "DAB-stop"):
            # check if the DAB player is already playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.system("sudo kill -9 "+process_find_lowest(audio_conf["source"], audio_conf["sink_uuid"]))
                return "DAB player stopped!"
            else:
                return "ERROR: Nothing stopped - DAB player is not playing or not on this SINK!"
        if (audio_conf["button"] == "DAB-tuneup"):
            # check if the DAB player is playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
                os.system("echo -n $'\e'\[A > dabin.pipe")
                return ""
            else:
                return "ERROR: Can't tune up - DAB player is not playing or not on this SINK!"
        if (audio_conf["button"] == "DAB-tunedown"):
            # check if the DAB player is playing
            if (process_source_playing(audio_conf["source"]) == audio_conf["sink_uuid"]):
                os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
                os.system("echo -n $'\e'\[B > dabin.pipe")
                return ""
            else:
                return "ERROR: Can't tune down - DAB player is not playing or not on this SINK!"



        return repr(audio_conf)+" - "+process_source_playing(audio_conf["source"])

    return 'ERROR: wrong value!'



# AUDIOOUTPUTS - SDCARD
##########################
@app.route('/sdcard', methods=['GET', 'POST'])
def raspi_sdcard():
    sink = request.args.get('sink') # sink where to play -> if empty -> default sink
    path = request.args.get('path') # show files in path
    file = request.args.get('file') # play this file
    options = request.args.get('options') # play options
    cmd = request.args.get('cmd') # file command

    os.chdir(os.path.dirname(os.path.realpath(__file__))+'/MUSIC/') # change working directory

    if cmd != None:
        form_keys = list(request.form.keys())
        form_values = list(request.form.values())
        form = dict(zip(list(request.form.keys()), list(request.form.values())))

        if cmd == "delete":
            try:
                rmtree(file)
            except OSError as e:
                pass
            return redirect(str(url_for('raspi_sdcard'))+"?path="+path+"&sink="+sink+"&options="+options)

        elif cmd == "upload":
            os.chdir(form['path'])
            files = request.files.getlist("file")
            for file in files:
                if file.filename != '':
                    file.save(file.filename)
            return redirect(str(url_for('raspi_sdcard'))+"?path="+path+"&sink="+sink+"&options="+options)

        elif cmd == "create":
            Path(os.path.join(form['path'], form['dirname'])).mkdir(parents=True, exist_ok=True)
            return redirect(str(url_for('raspi_sdcard'))+"?path="+path+"&sink="+sink+"&options="+options)

    elif file == None:

        if sink == None:
            return 'ERROR: sink is not set!'
        if os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list short sinks | grep $'\t''"+sink+"'$'\t'") == 1:
            return 'ERROR: sink doesnt exists!'+repr(sink)
        current_sink = sink

        sdcard_path = '.'
        if path != None and path != '':
            sdcard_path = path
        else:
            sdcard_path += '/'

        real_path = os.path.realpath(sdcard_path)

        if not os.path.exists(sdcard_path):
            return 'ERROR: wrong path!'+repr(sdcard_path)

        output = '<head><title>HistoRPi - SDcard player</title></head><h2>SDcard player</h2>'
        #output += '- real path: '+real_path+'<br>'
        output += 'Current path: '+sdcard_path.removeprefix('.')+'<br>'
        output += 'Current sink: '+current_sink+'<br><hr>'
        output += '<a href="/sdcard?file='+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/*.*')+'&sink='+urllib.parse.quote_plus(sink)+'&options='+urllib.parse.quote_plus(options)+'">PLAY THE CURRENT DIRECTORY</a><br><br>'
        if sdcard_path != './':
            output += 'DIR  - <a href="/sdcard?path='+urllib.parse.quote_plus(os.path.dirname(os.path.dirname(sdcard_path))+'/')+'&sink='+urllib.parse.quote_plus(sink)+'&options='+urllib.parse.quote_plus(options)+'">..</a><br>'
        for item in os.listdir(sdcard_path):
            if os.path.isdir(os.path.join(sdcard_path, item)):
                output += '<button onclick="if (confirm(\'Delete directory <'+item+'>?\')) {location.href=\'./sdcard?cmd=delete&path='+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/')+'&sink='+urllib.parse.quote_plus(sink)+'&options='+urllib.parse.quote_plus(options)+'&file='+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/'+item+'/')+'\';}">X</button> - DIR  - <a href="/sdcard?path='+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/'+item+'/')+'&sink='+urllib.parse.quote_plus(sink)+'&options='+urllib.parse.quote_plus(options)+'">'+item+'</a><br>'
            else:
                output += '<button onclick="if (confirm(\'Delete file <'+item+'>?\')) {location.href=\'./sdcard?cmd=delete&path='+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/')+'&sink='+urllib.parse.quote_plus(sink)+'&options='+urllib.parse.quote_plus(options)+'&file='+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/'+item)+'\';}">X</button> - FILE - <a href="/sdcard?file='+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/'+item)+'&sink='+urllib.parse.quote_plus(sink)+'&options='+urllib.parse.quote_plus(options)+'">'+item+'</a><br>'
        output += "<hr><form action='./sdcard?cmd=create&path="+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/')+"&sink="+urllib.parse.quote_plus(sink)+"&options="+urllib.parse.quote_plus(options)+"' method='POST'>Create new directory: <input type='text' name='dirname' placeholder='directory name'><input type='hidden' name='path' value='"+real_path+"'><input type='submit' value='create'></form>"
        output += "<form method='POST' enctype='multipart/form-data' action='./sdcard?cmd=upload&path="+urllib.parse.quote_plus(sdcard_path.removesuffix('/')+'/')+"&sink="+urllib.parse.quote_plus(sink)+"&options="+urllib.parse.quote_plus(options)+"' method='POST'>Upload file: <input type='file' name='file' placeholder='file name' multiple=''><input type='hidden' name='path' value='"+real_path+"'><input type='submit' value='upload'></form>"
        output += "<a href='./'>Home</a>"

        return output

    else:
        path = file.removesuffix('*.*')
        if not os.path.exists(path):
            return 'ERROR: file doesnt exists!'+repr(path)
        if sink == None:
            return 'ERROR: sink is not set!'
        if os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list short sinks | grep $'\t''"+sink+"'$'\t'") == 1:
            return 'ERROR: sink doesnt exists!'+repr(sink)
        if options == None:
            options = ""

        # check if the same AUDIO SOURCE is already playing
        sink_uuid = process_source_playing("SD")
        if (sink_uuid != ""):
            return '<script>alert("ERROR: SD audio source is already playing on sink: '+sink_uuid+'"); location.href = "./";</script>'


        path = './MUSIC'+file.removeprefix('.')

        if path.endswith('*.*'):
            path = "'"+path.removesuffix('*.*')+"'*.*"
        else:
            path = "'"+path+"'"

        # play sdcard mplayer
        result = raspi_playSD(sink, path, options)

        # save path to sink config as track
        #os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
        config_file_change_value(os.path.join(os.path.dirname(os.path.realpath(__file__)), './audio_config/'+sink+'.conf'), "AU_controls-SD-track", path)

        # wait to mplayer load up
        while(process_sink_get_track(sink) == "" and result == "Started Playing..."):
            time.sleep(0.500)

        return '<script>alert("'+result+'"); location.href = "./";</script>'
        #return redirect(url_for('index'))
        #return 'Play this file: '+path+'<br>On this sink: '+str(sink)





##########################
# TRANSMITTERS
##########################
@app.route('/transmitters', methods=['POST'])
def raspi_transmitters():
    if request.method == 'POST':
        form_keys = list(request.form.keys())
        form_values = list(request.form.values())
        trans_conf = dict(zip(list(request.form.keys()), list(request.form.values())))


        # check values
        live_value = '1' if (os.system("ps cax | grep pifmrds") == 0 or os.system("ps cax | grep rpitx") == 0) else '0'

        if "TS_live" in trans_conf and "TS_trans" in trans_conf:
            if live_value != trans_conf["TS_live"]: # continue only if TS_live value has changed
                if trans_conf["TS_live"] == "1":
                    if trans_conf["TS_trans"] == "FM":
                        return raspi_transFM(trans_conf["TS_source"], trans_conf["TS_freq"], trans_conf["TS_desc-8ch"], trans_conf["TS_desc-long"])
                    elif trans_conf["TS_trans"] == "AM":
                        return raspi_transAM(trans_conf["TS_source"], trans_conf["TS_freq"])
                else:
                    return raspi_transStop()
        form_keys.remove("TS_live")


        # save values
        os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory

        new_content = ""
        with open(web_file, "a+") as file:
            pass
        with open(web_file, "r+") as file:
            file.seek(0,0)
            for line in file:
                line = line.strip()

                if line.startswith("TS_"):
                    if len(form_keys) != 0:
                        for key in form_keys:
                            new_content += key+'='+trans_conf[key]+'\n'
                        form_keys.clear()

                else:
                    new_content += line+'\n'


            if len(form_keys) != 0:
                for key in form_keys:
                    new_content += key+'='+trans_conf[key]+'\n'
                form_keys.clear()


            file.seek(0,0)
            file.write(new_content)
            file.truncate()



        return ''#+repr("TS_live" in trans_conf)

    return 'ERROR: invalid request!'





##########################
# AUDIO SOURCES PLAYERS - PROCESSES
##########################
@app.route('/procs')
def raspi_procs(source="",sink=""):
    try:
        output = ""

        result = subprocess.check_output("pgrep -P "+str(os.getpid()), shell=True)
        child_process_ids = [int(line) for line in result.splitlines()]
        output += repr(child_process_ids)
        result = subprocess.check_output("ps -p "+str(os.getpid())+" -o args | tail -n 1", shell=True)
        output += str(result.decode())+"<br>"

        for child in child_process_ids:
            result = subprocess.run("pgrep -P "+str(child), shell=True, capture_output=True, text=True, check=False)
            child_process_ids = [int(line) for line in result.stdout.splitlines()]
            output += str(child)+" - "+repr(child_process_ids)
            result = subprocess.check_output("ps -p "+str(child)+" -o args | tail -n 1", shell=True)
            output += str(result.decode())+"<br>"





            for child in child_process_ids:
                result = subprocess.run("pgrep -P "+str(child), shell=True, capture_output=True, text=True, check=False)
                child_process_ids = [int(line) for line in result.stdout.splitlines()]
                output += str(child)+" -- "+repr(child_process_ids)
                result = subprocess.check_output("ps -p "+str(child)+" -o args | tail -n 1", shell=True)
                output += str(result.decode())+"<br>"

                for child in child_process_ids:
                    result = subprocess.run("pgrep -P "+str(child), shell=True, capture_output=True, text=True, check=False)
                    child_process_ids = [int(line) for line in result.stdout.splitlines()]
                    output += str(child)+" -- "+repr(child_process_ids)
                    result = subprocess.check_output("ps -p "+str(child)+" -o args | tail -n 1", shell=True)
                    output += str(result.decode())+"<br>"





        return output

    except subprocess.CalledProcessError as e:
        return "procs error:\n" + repr(e)
    return repr(child_process_ids)



def process_source_playing(audio_source): # returns sink name or ""
    try:
        result = subprocess.check_output("pgrep -P "+str(os.getpid()), shell=True)
        child_process_ids = [int(line) for line in result.splitlines()]

        for child in child_process_ids:
            result = subprocess.check_output("ps -p "+str(child)+" -o args | tail -n 1", shell=True)

            if str(result.decode()).startswith("/bin/sh -c echo '"+audio_source+"' ; "):
                sink_uuid = str(result.decode()).split(';',2)
                sink_uuid = sink_uuid[1].split('\'',2)
                return sink_uuid[1]

    except subprocess.CalledProcessError as e:
        return "procs error:\n" + repr(e)

    return ""


def process_sink_playing(sink): # returns audio source name or ""
    try:
        result = subprocess.check_output("pgrep -P "+str(os.getpid()), shell=True)
        child_process_ids = [int(line) for line in result.splitlines()]

        for child in child_process_ids:
            result = subprocess.check_output("ps -p "+str(child)+" -o args | tail -n 1", shell=True)

            sink_uuid = str(result.decode()).split(';',2)

            if len(sink_uuid) < 3:
                continue

            sink_uuid = sink_uuid[1].split('\'',2)
            if len(sink_uuid) < 3:
                continue

            if sink_uuid[1] == sink:
                audio_source = str(result.decode()).split(';',1)
                audio_source = audio_source[0].split('\'',2)
                return audio_source[1]

    except subprocess.CalledProcessError as e:
        return "procs error:\n" + repr(e)

    return ""


def process_sink_get_track(sink): # returns track name or ""
    try:
        # source: https://askubuntu.com/questions/813951/how-can-i-query-mplayer-about-the-currently-playing-song
        child = process_find_lowest("SD",sink)
        if child == "":
            return ""
        result = subprocess.check_output("lsof -c mplayer | grep --color=never -e '"+str(child)+".*/web-server/MUSIC/' -e '/web-server/MUSIC/.*"+str(child)+"' | awk -F\"/\" '{ print $NF; }' | cut -d'.' -f1", shell=True)
        return str(result.decode().strip())
    except subprocess.CalledProcessError as e:
        return "procs error:\n" + repr(e)

    return ""


def process_find_lowest(source="", sink=""):
    try:
        result = subprocess.check_output("pgrep -P "+str(os.getpid()), shell=True)
        child_process_ids = [int(line) for line in result.splitlines()]

        for child in child_process_ids:
            result = subprocess.run("pgrep -P "+str(child), shell=True, capture_output=True, text=True, check=False)
            child_process_ids = [int(line) for line in result.stdout.splitlines()]
            result = subprocess.check_output("ps -p "+str(child)+" -o args | tail -n 1", shell=True)


            if str(result.decode()).startswith("/bin/sh -c echo '"+source+"' ; echo '"+sink+"' ; "):

                if len(child_process_ids) > 0:
                    child = child_process_ids[0]
                    result = subprocess.run("pgrep -P "+str(child), shell=True, capture_output=True, text=True, check=False)
                    child_process_ids = [int(line) for line in result.stdout.splitlines()]

                    if len(child_process_ids) > 0:
                        child = child_process_ids[0]
                        #result = subprocess.run("pgrep -P "+str(child), shell=True, capture_output=True, text=True, check=False)
                        #child_process_ids = [int(line) for line in result.stdout.splitlines()]

                return str(child) ## return the lowest subprocess PID


    except subprocess.CalledProcessError as e:
        return "procs error:\n" + repr(e)

    return "" # SOURCE is not playing on SINK





##########################
# AUDIO SOURCES PLAYERS - START COMMANDS
##########################
def raspi_playSD(sink, path, options):
    if process_source_playing("SD") == "":
        try:
            """
            mplayer './MUSIC/'*.*
            mplayer -loop 0 -shuffle ./MUSIC/*.*

            mkfifo /tmp/mplayercontrol.pipe
            [[ -p "/tmp/mplayercontrol.pipe" ]]
            rm /tmp/mplayercontrol.pipe

            https://stackoverflow.com/questions/52462131/how-to-make-mplayer-continue-after-pausing-it-via-a-named-pipe
            https://stackoverflow.com/questions/65537920/mplayer-change-track-play-pause-with-command-line
            https://stackoverflow.com/questions/20245675/how-to-test-if-a-named-pipe-exists
            http://www.mplayerhq.hu/DOCS/tech/slave.txt

            next
            echo "pt_step 1" > /tmp/mplayercontrol.pipe

            previous
            echo "pt_step -1" > /tmp/mplayercontrol.pipe

            toggle pause/resume
            echo "pause" > /tmp/mplayercontrol.pipe

            disable loop
            loop -1
            """
            # create named pipe if doesn't exist
            filename = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mplayercontrol.pipe')
            if os.system('[ -p "'+filename+'" ]') != 0:
                os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 rm "+filename)
                os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 mkfifo "+filename)

            play_command = "echo 'SD' ; echo '"+sink+"' ; sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 mplayer -slave -input file="+filename+" "+options+" -ao pulse::"+sink+" "+path+""
            subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
            return 'Started Playing...'
        except subprocess.CalledProcessError as e:
            return "Playing error:\n" + repr(e)
    return 'ERROR: Still playing!'


def raspi_playURL(sink, url):
    if process_source_playing("URL") == "":
        try:
            play_command = "echo 'URL' ; echo '"+sink+"' ; sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 mplayer -ao pulse::"+sink+" '"+url+"'"
            subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
            return 'Started Playing...'
        except subprocess.CalledProcessError as e:
            return "Playing error:\n" + repr(e)
    return 'ERROR: Still playing!'


def raspi_playFM(sink, freq):
    if process_source_playing("FM") == "":
        try:
            play_command = "echo 'FM' ; echo '"+sink+"' ; sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 rtl_fm -f "+freq+"e6 -s 200000 -r 48000 | sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 ffmpeg -use_wallclock_as_timestamps 1 -f s16le -ac 1 -ar 48000 -i - -ac 2 -f pulse -device '"+sink+"' 'stream-title'"
            subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
            return 'Started Playing...'
        except subprocess.CalledProcessError as e:
            return "Playing error:\n" + repr(e)
    return 'ERROR: Still playing!'


def raspi_playDAB(sink, channel, station=''):
    if process_source_playing("DAB") == "":
        try:
            # create named pipe if doesn't exist
            pipein = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'dabin.pipe')
            if os.system('[ -p "'+pipein+'" ]') != 0:
                os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 rm "+pipein)
                os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 mkfifo "+pipein)

            # create named pipe if doesn't exist
            pipeout = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'dabout.pipe')
            if os.system('[ -p "'+pipeout+'" ]') != 0:
                os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 rm "+pipeout)
                os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 mkfifo "+pipeout)


            ## play_command = "echo 'DAB' ; echo '"+sink+"' ; sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 dab-rtlsdr-4 -C "+channel+" -P '"+station+"' -D 60 -d 60 | sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 ffmpeg -use_wallclock_as_timestamps 1 -f s16le -ac 1 -ar 48000 -i - -ac 2 -f pulse -device '"+sink+"' 'stream-title'"

            play_command = "echo 'DAB' ; echo '"+sink+"' ; export TERM=linux ; sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 terminal-DAB-rtlsdr -C "+channel+" -Q 0<dabin.pipe"
            proc = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__)), stdout=subprocess.PIPE) # change working directory to this script path


            ensemble = ""
            data = ""
            byte = b''
            while True:
                byte = proc.stdout.read(1)
                if not byte:
                    break
                #print("byte:", byte)
                if byte == b'*':
                    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
                    os.system("echo -n $'\e'\[A > dabin.pipe")
                    # https://stackoverflow.com/questions/17002403/simulate-up-arrow-press-in-linux
                    # https://www.linuxquestions.org/questions/linux-newbie-8/bash-echo-the-arrow-keys-825773/
                    break

                try:
                    if byte != b'\x1b':
                        data += str(byte.decode("utf-8"))
                except:
                    pass

                if data.endswith("[H"):
                    if "Ensemble: " in data:
                        ensemble = (data.split("Ensemble: ")[1]).removesuffix("[H")
                    print(data)
                    data = ""
            #print("DONE")


            if byte == b'*':
                return 'Started Playing...\nMultiplex: '+ensemble
            return 'Playing stoped: error!'
        except subprocess.CalledProcessError as e:
            return "Playing error:\n" + repr(e)
    return 'ERROR: Still playing!'


def raspi_playBT(sink):
    if process_source_playing("BT") == "":
        try:
            play_command = "echo 'BT' ; echo '"+sink+"' ; ./LIBS/promiscuous-bluetooth-audio-sinc/a2dp-agent"
            subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
            return 'Started Playing...'
        except subprocess.CalledProcessError as e:
            return "Playing error:\n" + repr(e)
    return 'ERROR: Still playing!'





##########################
# TRANSMITTERS - START COMMANDS
##########################
def raspi_transFM(sink="TransmittersSink", freq="89.0", desc_short="HistoRPi", desc_long="HistoRPi: live FM-RDS transmission from the RaspberryPi"):
    if os.system("ps cax | grep pifmrds") != 0:
        try:
            ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
            ## user id: id -u
            ## user id = 1000
            play_command = "echo 'transFM' ; sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 ffmpeg -use_wallclock_as_timestamps 1 -f pulse -i "+sink+".monitor -ac 2 -f wav - | sudo ./LIBS/rpitx/pifmrds -ps '"+desc_short+"' -rt '"+desc_long+"' -freq "+freq+" -audio -"
            subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path

            return 'Started transmitting...'
        except subprocess.CalledProcessError as e:
            return "Transmitting error:\n" + repr(e)
    return 'ERROR: Still transmitting!'


def raspi_transAM(sink="TransmittersSink", freq="1.6"):
    freq = float(freq)
    freq *= 1000
    freq = '%.0f' % freq

    if os.system("ps cax | grep rpitx") != 0:
        try:
            ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
            ## user id: id -u
            ## user id = 1000
            play_command = "echo 'transAM' ; sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 ffmpeg -use_wallclock_as_timestamps 1 -f pulse -i "+sink+".monitor -ac 1 -ar 48000 -acodec pcm_s16le -f wav - | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo ./LIBS/rpitx/rpitx -i- -m RF -f "+freq
            subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path

            return 'Started transmitting...'
        except subprocess.CalledProcessError as e:
            return "Transmitting error:\n" + repr(e)
    return 'ERROR: Still transmitting!'





##########################
# SETTINGS
##########################
@app.route('/AUstop')
def raspi_audiooutputsStop():
    if (os.system("ps cax | grep mplayer") == 0 or os.system("ps cax | grep rtl_fm") == 0 or os.system("ps cax | terminal-DAB-rtlsdr") == 0 or os.system("ps cax | grep a2dp-agent") == 0):
        os.system("sudo killall mplayer")
        os.system("sudo killall rtl_fm")
        os.system("sudo killall terminal-DAB-rtlsdr")
        os.system("sudo killall -SIGINT a2dp-agent")
        return 'Stopped!'
    return 'Nothing playing!'


@app.route('/TRstop')
def raspi_transmittersStop():
    if (os.system("ps cax | grep pifmrds") == 0 or os.system("ps cax | grep rpitx") == 0):
        os.system("sudo killall pifmrds")
        os.system("sudo killall rpitx")
        return 'Stopped transmitting!'
    return 'Nothing transmitting!'


@app.route('/removewifi/<uuid>/ssid/<ssid>', strict_slashes=True)
def raspi_removewifi(uuid="", ssid=""):
    if os.system("sudo nmcli connection delete "+str(uuid)) == 0:
        return 'WIFI connection "'+ssid+'" deleted!\nReload page to see changes.'
    else:
        return 'ERROR while deleting WIFI connection: '+ssid+' ('+uuid+')'

@app.route('/disconnect')
def raspi_disconnect():
    try:
        result = subprocess.check_output("nmcli --colors no device wifi show-password | grep 'SSID:' | cut -d ':' -f 2", shell=True)
        wifi_conn = result.decode().strip()
        os.system("sudo nmcli connection delete "+str(wifi_conn))
        os.system("sudo reboot")
    except subprocess.CalledProcessError as e:
        return "ERROR:\n" + repr(e.output)
    return 'Current WIFI connection deleted!'

@app.route('/savewifi', methods=['POST'])
def raspi_savewifi():
    if request.method == 'POST':
        print(*list(request.form.keys()), sep = ", ")
        ssid = request.form['ssid']
        password = request.form['password']

        os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory

        new_content = ""

        with open(conf_file, "a+") as file:
            pass
        with open(conf_file, "r+") as file:
            for line in file:
                line = line.strip()
                if line.startswith("WIFI_SSID"):
                    new_content += 'WIFI_SSID="'+ssid+'"\n'
                elif line.startswith("WIFI_PASSWORD"):
                    new_content += 'WIFI_PASSWORD="'+password+'"\n'
                else:
                    new_content += line+'\n'
            file.seek(0,0)
            file.write(new_content)
            file.truncate()

    return "New settings saved successfully!\nReboot to apply."

@app.route('/disablevoiceip')
def raspi_disablevoiceip():
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
    iptospeechwas = False
    new_content = ""
    with open(conf_file, "a+") as file:
        pass
    with open(conf_file, "r+") as file:
        for line in file:
            line = line.strip()
            if line.startswith("IPtoSPEECH"):
                new_content += 'IPtoSPEECH=false\n'
                if line == "IPtoSPEECH=true":
                    iptospeechwas = True
            else:
                new_content += line+'\n'
        if iptospeechwas:
            file.seek(0,0)
            file.write(new_content)
            file.truncate()
    return 'IP to Speech is now: OFF (was '+str('ON' if iptospeechwas else 'OFF')+')'

@app.route('/reboot')
def raspi_reboot():
    try:
        os.system("sudo reboot")
    except:
        return 'ERROR while rebooting!'
    return 'Reboot!'

@app.route('/shutdown')
def raspi_shutdown():
    try:
        os.system("sudo shutdown now")
    except:
        return 'ERROR while shuting down!'
    return 'Shutdown!'





##########################
# MAIN
##########################
@app.route('/startup')
def raspi_startup():
    ### this function runs right after the Flask starts and runs on it's process

    ## create config dir
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
    Path("./audio_config/").mkdir(parents=True, exist_ok=True)
    ## create music dir
    Path("./MUSIC/").mkdir(parents=True, exist_ok=True)

    ## check autoplays
    check_autoplays()


def raspi_run_startup():
    time.sleep(1)
    while os.system("curl 127.0.0.1/startup") != 0:
        time.sleep(1)


def main():
    # commands to run before webserver starts

    # wait for pulseaudio
    while os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl info") != 0:
        time.sleep(1)
        print("WAITING FOR PULSEAUDIO...")
    print("PULSEADUIO READY!")
    # create virtual audio device for transmitters
    if os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list sources short | grep --color=never TransmittersSink") != 0:
        os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pacmd load-module module-null-sink sink_name=TransmittersSink")
        os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pacmd update-sink-proplist TransmittersSink device.description=TransmittersSink")
        os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pacmd update-source-proplist TransmittersSink.monitor device.description='Monitor of TransmittersSink'")
        os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-default-sink "+config_file_get_value(web_file, "AU_default"))

        # create a process - run After startup function
        process = Process(target=raspi_run_startup)
        process.start() # run the process


if __name__ == '__main__':
    main()
    app.run(debug=True, host='0.0.0.0', port=80)

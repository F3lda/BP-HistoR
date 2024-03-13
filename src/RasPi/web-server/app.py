from flask import Flask,request
import subprocess
from threading import Thread
import os
import time
# user repr() as var_dump()


app = Flask(__name__)

wifi_device = "wlan0"
conf_file = "device.conf"
web_file = "web.conf"



def list_join_span(array, separator, span):
    # https://stackoverflow.com/questions/1621906/is-there-a-way-to-split-a-string-by-every-nth-separator-in-python
    return [separator.join(array[i:i+span]) for i in range(0, len(array), span)]



def web_config_change_value(key, value):
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
    
    value_changed = False
    new_content = ""
    with open(web_file, "r+") as file:
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


def web_config_get_value(key):
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
    
    value = None
    with open(web_file, "r+") as file:
        for line in file:
            line = line.strip()
            if line.startswith(key):
                val = line.split('=',1)
                if len(val) == 2:
                    value = val[1]
    return value



def read_web_config():
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory

    settings_vars = ["TS_live", "TS_trans", "TS_desc-8ch", "TS_desc-long", "TS_freq", "TS_source", "TS_autoplay"]

    file_items = {}
    with open(web_file, "r+") as file:
        for line in file:
            line = line.strip()
            item = line.split('=',1)
            if len(item) == 2:
                file_items[item[0]] = item[1]
    return file_items



def check_web_config_states():
    # check FM transmitters
    if (os.system("ps cax | grep pifmrds") == 0 or os.system("ps cax | grep rpitx") == 0):
        web_config_change_value("TS_live", "1")
    else:
        web_config_change_value("TS_live", "0")


def check_autoplay():
    if web_config_get_value("TS_autoplay") == '1':
        transmitter = web_config_get_value("TS_trans")
        if transmitter == "FM":
            return raspi_transFM(web_config_get_value("TS_source"), web_config_get_value("TS_freq"), web_config_get_value("TS_desc-8ch"), web_config_get_value("TS_desc-long"))
        elif transmitter == "AM":
            return raspi_transAM(web_config_get_value("TS_source"), web_config_get_value("TS_freq"))


@app.route('/')
def index():
    ## Disable IPtoSPEECH
    raspi_disablevoiceip()
    
    ## check config states
    check_web_config_states()
    
    ## read config file
    web_config = read_web_config()
    
    
    
    
    
    dropdowndisplay = ""
    dropdowndisplay += """



    <h1>HistoRPi - audio streaming device for historic radios</h1><hr>


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


    default_sink = ''
    try:
        result = subprocess.check_output("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl info | grep --color=never 'Default Sink: '", shell=True)
        default_sink = result.decode().strip().removeprefix("Default Sink: ")
        #dropdowndisplay += "Default sink: "+repr(default_sink)+"<br>"
    except subprocess.CalledProcessError as e:
        dropdowndisplay += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)

    device_sinks = []
    try:
        result = subprocess.check_output("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list sinks | grep --color=never 'Sink \|^[[:space:]]Volume: \|^[[:space:]]Description: \|^[[:space:]]Name: \|^[[:space:]]State: '", shell=True)
        result = result.decode().strip()
        result = result.split('\n')
        curr_sink = 0
        device_sink = []
        for i, line in enumerate(result):
            #device_descs.append(desc.strip().removeprefix("Description: "))
            if line.startswith('Sink #'):
                if curr_sink != int(line.removeprefix('Sink #')):
                    device_sinks.append(device_sink.copy())
                    device_sink.clear()
                    curr_sink+=1
                device_sink.append(line.removeprefix('Sink #'))
            else:
                line = line.strip()
                if line.startswith('State: '):
                    line = line.removeprefix('State: ')
                elif line.startswith('Name: '):
                    line = line.removeprefix('Name: ')
                    if line == default_sink:
                        device_sink.append('checked')
                    else:
                        device_sink.append('')
                elif line.startswith('Description: '):
                    line = line.removeprefix('Description: ')
                elif line.startswith('Volume: '):
                    line = line.split('/')
                    if len(line) > 1:
                        line = line[1].strip().removesuffix('%')
                    else:
                        line = '(unknown)'
                device_sink.append(line)
            #dropdowndisplay += repr(line)+"<br>"
        device_sinks.append(device_sink.copy())

        #dropdowndisplay += repr(device_sinks)+"<br>"

    except subprocess.CalledProcessError as e:
        dropdowndisplay += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)



    for sink in device_sinks:
        #dropdowndisplay += repr(sink)+"<br>"
        dropdowndisplay +=f"""
  		<tr>
            <td><input type="radio" name="default" {sink[2]}></td>
            <td>{sink[0]}</td>
            <td>{sink[4]}</td>
            <td>{sink[1]}</td>
            <td><input type="number" name="tentacles" min="1" max="120" value="{sink[5]}" size="5">%</td>
            <td>
                <select class="cls-controls" name="sink-{sink[0]}-source" onchange="change_controls(this.name, {sink[0]}, this.value)">
                    <option value="SD">SDcard player</option>
                    <option value="URL">URL player</option>
                    <option value="FM">FM radio</option>
                    <option value="BT">Bluetooth</option>
                    <option value="DAB">DAB radio</option>
                </select>
            </td>
            <td><input type="checkbox" checked disabled></td>
            <td><input type="checkbox" checked></td>
            <td id="controls-{sink[0]}"></td>
        </tr>
    """


    dropdowndisplay += """
    </table>
    </form>
    <script>
    window.addEventListener("load", function(event) {
        var list = document.getElementsByClassName("cls-controls") ;
        for (let item of list) {
            item.dispatchEvent(new Event('change'));
        }
    });

    function change_controls(name, id, value)
    {
        var controls = document.getElementById("controls-"+id)
        if (value == "SD") {
            controls.innerHTML = `<input type="submit" value="<" title="previous">
            <input type="submit" value="II" title="pause">
            <input type="submit" value="|>" title="play">
            <input type="submit" value="O" title="stop">
            <input type="submit" value=">" title="next"> -
            repeat: <input type="checkbox" checked>
            shuffle: <input type="checkbox" checked>
            autoplay: <input type="checkbox" checked>
            - <a href="#select">select track</a>`
        } else if (value == "URL") {
            controls.innerHTML = `<input type="text" value="" placeholder="URL" title="URL">
            <input type="submit" value="|>" title="play">
            <input type="submit" value="O" title="stop"><!--- maybe TODO: URL list; --->`
        } else if (value == "FM") {
            controls.innerHTML = `<input type="number" value="" placeholder="FREQ" title="FREQ">
            <input type="submit" value="<" title="tune down">
            <input type="submit" value="|>" title="play">
            <input type="submit" value="O" title="stop">
            <input type="submit" value=">" title="tune up">`
        } else if (value == "BT") {
            controls.innerHTML = `<input type="text" value="" placeholder="BT NAME" title="BT NAME">
            <input type="submit" value="ON" title="play">
            <input type="submit" value="OFF" title="stop">`
        } else if (value == "DAB") {
            controls.innerHTML = `<input type="text" value="" placeholder="CHANNEL" title="CHANNEL">
            <input type="submit" value="<" title="tune down">
            <input type="submit" value="|>" title="play">
            <input type="submit" value="O" title="stop">
            <input type="submit" value=">" title="tune up">`
        }
        console.log(name + ": " + value)
    }
    </script>
    """




    dropdowndisplay += f"""
    <h2>Transmitters</h2>
    <pre>!!! WARNING !!! - RaspberryPi's WiFi connection is interfered with AM transmission -> use connection over Ethernet cable !!! (Use cable connection from Switch rather then from Wifi device!)</pre>
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
                    <input type="hidden" name="TS_live" value="{web_config["TS_live"]}"><input type="checkbox" onclick="this.previousElementSibling.value=1-this.previousElementSibling.value" {'checked' if web_config["TS_live"] == "1" else ''}><span>ON AIR</span>
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
                    {''.join(["<option value='"+sink[3]+"' selected>["+sink[0]+"] "+sink[4]+"</option>" if web_config["TS_source"] == sink[3] else "<option value='"+sink[3]+"'>["+sink[0]+"] "+sink[4]+"</option>" for sink in device_sinks])}
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




    <h2>Tests</h2>
    Audio<br>
    <a href="./play">Play</a>
    <a href="./playradio">Play Internet Radio</a>
    <a href="./playFM">Play FM Radio (107.0 MHz)</a>
    <a href="./playDAB">Play DAB Radio (DAB TOP40)</a>
    <a href="./playBT">Play Bluetooth</a>
    <br>
    <a href="./stop">Stop</a>
    <br>Volume<br>
    <a href="./volume">Volume</a>
    <a href="./volumeUp">Volume Up</a>
    <a href="./volumeDown">Volume Down</a>
    <br>Transmitters<br>
    <a href="./transFM">Play FM (89 MHz)</a>
    <a href="./transAM7000">Play AM (10 MHz)</a>
    <a href="./transAM1600">Play AM (1.6 MHz)</a>
    <br>
    <a href="./transStop">Stop</a>
    <br><br><pre>WARNING: FM radio, DAB radio and Bluetooth are not working while transmitting from RaspberryPi using SDR!!! (there is probably interference)</pre>
    <br><br><br><br><br><br>
    """








    #
    # SETTINGS
    ##########################
    dropdowndisplay += """
    <h2>Settings</h2>
    <h3>Network state</h3>
    IP addresses:
    """
    try:
        result = subprocess.check_output("hostname -I", shell=True)
        dropdowndisplay += "<strong>"+str(', '.join(result.decode().strip().split(' ')))+"</strong>"
    except subprocess.CalledProcessError as e:
        dropdowndisplay += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)

    dropdowndisplay += "<br>Devices:<br>"
    try:
        result = subprocess.check_output("nmcli --colors no -m multiline connection show --active", shell=True)
        cells_list = result.decode().strip().split("\n")
        connections_list = list_join_span(cells_list, "\n", 4)

        dropdowndisplay += "<table border='1'><tr><th>DEVICE</th><th>TYPE</th><th>UUID</th><th>NAME</th></tr>"
        for device in connections_list:
            dropdowndisplay += "<tr>"
            device_cells = device.split('\n')
            device_cells.reverse()
            if len(device_cells) == 4:
                for device_cell in device_cells:
                    dropdowndisplay += "<td>"+device_cell.split(":",1)[1].strip()+ "</td>"
            dropdowndisplay += "</tr>"
        dropdowndisplay += "</table>"
    except subprocess.CalledProcessError as e:
        dropdowndisplay += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)



    dropdowndisplay += """
    <h3>WiFi saved connections</h3>
    """
    try:
        result = subprocess.check_output("nmcli --colors no -m multiline connection show", shell=True)
        cells_list = result.decode().strip().split("\n")
        connections_list = list_join_span(cells_list, "\n", 4)

        dropdowndisplay += "<table border='1'><tr><th>NAME</th><th>UUID</th><th>TYPE</th><th>DEVICE</th><th>X</th></tr>"
        for connection in connections_list:
            dropdowndisplay += "<tr>"
            connection_cells = connection.split('\n')
            if len(connection_cells) == 4 and connection_cells[0].replace(" ", "") != "NAME:Hotspot" and connection_cells[2].replace(" ", "") == "TYPE:wifi":
                for connection_cell in connection_cells:
                    dropdowndisplay += "<td>"+connection_cell.split(":",1)[1].strip()+ "</td>"
                dropdowndisplay += "<td><a href='/removewifi/"+connection_cells[1].split(":",1)[1].strip()+"/ssid/"+connection_cells[0].split(":",1)[1].strip()+"'>delete connection</a></td>"
            dropdowndisplay += "</tr>"
        dropdowndisplay += "</table>"

    except subprocess.CalledProcessError as e:
        dropdowndisplay += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)

    try:
        result = subprocess.check_output("nmcli --colors no device wifi show-password | grep 'SSID:' | cut -d ':' -f 2", shell=True)
        dropdowndisplay += """
        <br>Connected to WiFi: <strong>"""+str(result.decode().strip())+"""</strong><br>
        """
    except subprocess.CalledProcessError as e:
        dropdowndisplay += "<pre>command '{}' return with error (code {}): {}</pre>".format(e.cmd, e.returncode, e.output)

    dropdowndisplay += """
    <a href="./disconnect">Delete current WiFi connection & Reboot</a>
    """



    saved_SSID = ""
    saved_PASS = ""
    os.chdir(os.path.dirname(os.path.realpath(__file__))) # change working directory
    with open(conf_file, "r+") as file:
        for line in file:
            line = line.strip()
            if line.startswith("WIFI_SSID"):
                saved_SSID = line.removeprefix("WIFI_SSID=\"").removesuffix("\"")
            elif line.startswith("WIFI_PASSWORD"):
                saved_PASS = line.removeprefix("WIFI_PASSWORD=\"").removesuffix("\"")

    dropdowndisplay += """
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
        document.body.addEventListener("change", function(e) {
            if (e.target.form != null && e.target.form.id != null) {
                if (e.target.form.id == "Transmitters" || e.target.form.id == "AudioOutputs") {

                    formData = new FormData();
                    formData.append('name', e.target.name);
                    formData.append('value', e.target.value);
                    formData = new FormData(e.target.form);

                    httpPOST(e.target.form.action, formData)

                    e.preventDefault();
                }
            }
        });

        document.body.addEventListener("submit", function(e) {

            if (e.target != null && e.target.id != null) {
                if (e.target.id == "SaveWifi") {
                    httpPOST(e.target.action, new FormData(e.target));
                }
            }
            e.preventDefault();
        });

        document.body.addEventListener("click", function(e) {
            if (e.target && e.target.nodeName == "A") {

                httpGET(e.target.href);

                e.preventDefault();
            }
        });

        async function httpGET(url) {
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
            document.body.style.cursor = 'wait';
            try {
                const response = await fetch(action, {
                    method: "POST",
                    body: data
                });
                const result = await response.text();

                console.log("Success: " + result);
                if (result != "") {
                    alert(result);
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
    return dropdowndisplay



##########################
# AUDIOOUTPUTS
##########################
@app.route('/audiooutputs', methods=['POST'])
def raspi_audiooutputs():
    if request.method == 'POST':
        form_keys = list(request.form.keys())
        form_values = list(request.form.values())
        for i, value in enumerate(form_values):
            if (value == ""):
                return 'ERROR: wrong value in input: '+form_keys[i]
        return repr(form_keys)+repr(form_values)


    return 'ERORR: wrong value!'










##########################
# TRANSMITTERS
##########################
@app.route('/transmitters', methods=['POST'])
def raspi_transmitters():
    if request.method == 'POST':
        form_keys = list(request.form.keys())
        form_values = list(request.form.values())
        
        live_value = web_config_get_value("TS_live")

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
                        for i, key in enumerate(form_keys):
                            new_content += key+'='+form_values[i]+'\n'
                        form_keys.clear()

                else:
                    new_content += line+'\n'


            if len(form_keys) != 0:
                for i, key in enumerate(form_keys):
                    new_content += key+'='+form_values[i]+'\n'
                form_keys.clear()


            file.seek(0,0)
            file.write(new_content)
            file.truncate()
        
        # check values
        form_keys = list(request.form.keys())
        form_values = list(request.form.values())
        # transmitters
        if "TS_live" in form_keys and "TS_trans" in form_keys:
            if live_value != form_values[0]: # continue only if TS_live value has changed
                if form_values[0] == "1":
                    if form_keys[1] == "TS_trans" and form_values[1] == "FM":
                        return raspi_transFM(form_values[5], form_values[4], form_values[2], form_values[3])
                    elif form_keys[6] == "TS_trans" and form_values[6] == "AM":
                        return raspi_transAM(form_values[4], form_values[3])
                else:
                    return raspi_transStop()
        
        return ''

    return 'ERORR: invalid request!'












##########################
# TESTS
##########################

AUDIOplayingProcess = None

@app.route('/play')
def raspi_play():
    global AUDIOplayingProcess

    if AUDIOplayingProcess != None:
        if AUDIOplayingProcess.poll() != None:
            AUDIOplayingProcess = None

    if AUDIOplayingProcess == None:
        #play_command = ["nmcli", "--colors", "no", "device", "wifi", "connect", ssid, "ifname", wifi_device]
        #play_command = ["python", "--version"]
        #play_command = ["mplayer", "-ao", "alsa:device=hw=0.0", "./MUSIC/Creedence Clearwater Revival - Fortunate Son (Official Music Video).mp3"]
        #AUDIOplayingProcess = subprocess.Popen(play_command, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 mplayer -ao pulse::TransmittersSink './MUSIC/Creedence Clearwater Revival - Fortunate Son (Official Music Video).mp3'"
        AUDIOplayingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path

        return 'Started Playing...'
    return 'Still playing!'



@app.route('/playradio')
def raspi_playradio():
    global AUDIOplayingProcess

    if AUDIOplayingProcess != None:
        if AUDIOplayingProcess.poll() != None:
            AUDIOplayingProcess = None

    if AUDIOplayingProcess == None:
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 mplayer -ao pulse::TransmittersSink https://ice5.abradio.cz/hitvysocina128.mp3"
        AUDIOplayingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
        return 'Started Playing...'
    return 'Still playing!'



@app.route('/playFM')
def raspi_playFM():
    global AUDIOplayingProcess

    if AUDIOplayingProcess != None:
        if AUDIOplayingProcess.poll() != None:
            AUDIOplayingProcess = None

    if AUDIOplayingProcess == None:
        #rtl_fm -f 107e6 -s 200000 -r 48000 | aplay -r 48000 -f S16_LE -t raw -c 2
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 rtl_fm -f 107e6 -s 200000 -r 48000 | mplayer -ao pulse::TransmittersSink -noconsolecontrols -cache 1024 -"
        AUDIOplayingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
        return 'Started Playing...'
    return 'Still playing!'



@app.route('/playDAB')
def raspi_playDAB():
    global AUDIOplayingProcess

    if AUDIOplayingProcess != None:
        if AUDIOplayingProcess.poll() != None:
            AUDIOplayingProcess = None

    if AUDIOplayingProcess == None:
        #dab-rtlsdr-4 -C 12D -P 'CRo' -D 60 -d 60 | aplay -r 48000 -f S16_LE -t raw -c 2
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 dab-rtlsdr-4 -C 12D -P 'CRo' -D 60 -d 60 | mplayer -ao pulse::TransmittersSink -noconsolecontrols -cache 1024 -"
        #play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 dab-rtlsdr-4 -C 8A -P 'DAB' -D 60 -d 60 | mplayer -ao pulse::TransmittersSink -noconsolecontrols -cache 1024 -"
        #play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 dab-rtlsdr-4 -C 8A -P 'DAB' -D 60 -d 60 | ffmpeg -loglevel error -i pipe: -c:a pcm_s16le -f s16le pipe: | mplayer -ao pulse::TransmittersSink -noconsolecontrols -cache 1024 -"
        AUDIOplayingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
        return 'Started Playing...'
    return 'Still playing!'



@app.route('/playBT')
def raspi_playBT():
    global AUDIOplayingProcess

    if AUDIOplayingProcess != None:
        if AUDIOplayingProcess.poll() != None:
            AUDIOplayingProcess = None

    if AUDIOplayingProcess == None:
        play_command = "./LIBS/promiscuous-bluetooth-audio-sinc/a2dp-agent"
        AUDIOplayingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
        return 'Started Playing...'
    return 'Still playing!'




@app.route('/stop')
def raspi_stop():
    global AUDIOplayingProcess

    if AUDIOplayingProcess != None:
        if AUDIOplayingProcess.poll() == None:
            AUDIOplayingProcess.terminate()
            os.system("sudo killall mplayer")
            os.system("sudo killall -SIGINT a2dp-agent")
            #os.system("sudo killall ffmpeg")
            return 'Stopped!'
        AUDIOplayingProcess = None
    return 'Nothing playing!'











# VOLUME
##########################

@app.route('/volumeUp')
def raspi_volumeUp():
    ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
    ## user id: id -u
    ## user id = 1000
    os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-sink-volume TransmittersSink +10%")

    output="Volume: "
    try:
        output += str(int(subprocess.check_output("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list sinks | grep '^[[:space:]]Volume:' | head -n $(( 1 )) | tail -n 1 | sed -e 's,.* \\([0-9][0-9]*\\)%.*,\\1,'", shell = True)))
    except subprocess.CalledProcessError as e:
        output += "pactl stdout output:\n" + repr(e.output)
    output+=" %"

    output += '<br>Volume up +5 %!'

    return output

@app.route('/volumeDown')
def raspi_volumeDown():
    ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
    ## user id: id -u
    ## user id = 1000
    os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-sink-volume TransmittersSink -10%")

    output="Volume: "
    try:
        output += str(int(subprocess.check_output("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list sinks | grep '^[[:space:]]Volume:' | head -n $(( 1 )) | tail -n 1 | sed -e 's,.* \\([0-9][0-9]*\\)%.*,\\1,'", shell = True)))
    except subprocess.CalledProcessError as e:
        output += "pactl stdout output:\n" + repr(e.output)
    output+=" %"

    output += '<br>Volume down -5 %!'

    return output


@app.route('/volume')
def raspi_volume():
    ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
    ## user id: id -u
    ## user id = 1000
    output="Volume: "
    try:
        output += str(int(subprocess.check_output("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list sinks | grep '^[[:space:]]Volume:' | head -n $(( 1 )) | tail -n 1 | sed -e 's,.* \\([0-9][0-9]*\\)%.*,\\1,'", shell = True)))
    except subprocess.CalledProcessError as e:
        output += "pactl stdout output:\n" + repr(e.output)
    output+=" %"
    return output








## TODO: wait for song end
def run_cmd_background(cmd):
    data = {'cmd': cmd}
    thr = Thread(target=run_async_func, args=[app, data])
    thr.start()
    return thr

def run_async_func(app, data):
    global AUDIOplayingProcess

    with app.app_context():
    # Your working code here!
        return_code = subprocess.Popen(data['cmd'], cwd=os.path.dirname(os.path.realpath(__file__)))
        if return_code == 0:
            print("Command executed successfully.")
        else:
            print("Command failed with return code", return_code)

        AUDIOplayingProcess = None
















# TRANSMITTERS
##########################

FMAMtransmittingProcess = None

@app.route('/transFM')
def raspi_transFM(sink="TransmittersSink", freq="89.0", desc_short="HistoRPi", desc_long="HistoRPi: live FM-RDS transmission from the RaspberryPi"):
    global FMAMtransmittingProcess

    if FMAMtransmittingProcess != None:
        if FMAMtransmittingProcess.poll() != None:
            FMAMtransmittingProcess = None

    if FMAMtransmittingProcess == None:

        ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
        ## user id: id -u
        ## user id = 1000
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 ffmpeg -use_wallclock_as_timestamps 1 -f pulse -i "+sink+".monitor -ac 2 -f wav - | sudo ./LIBS/rpitx/pifmrds -ps '"+desc_short+"' -rt '"+desc_long+"' -freq "+freq+" -audio -"
        FMAMtransmittingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path

        return 'Started transmitting...'
    return 'Still transmitting!'


@app.route('/transAM')
def raspi_transAM(sink="TransmittersSink", freq="1.6"):
    freq = float(freq)
    freq *= 1000
    freq = '%.0f' % freq
    
    global FMAMtransmittingProcess

    if FMAMtransmittingProcess != None:
        if FMAMtransmittingProcess.poll() != None:
            FMAMtransmittingProcess = None

    if FMAMtransmittingProcess == None:

        ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
        ## user id: id -u
        ## user id = 1000
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 ffmpeg -use_wallclock_as_timestamps 1 -f pulse -i "+sink+".monitor -ac 1 -ar 48000 -acodec pcm_s16le -f wav - | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo ./LIBS/rpitx/rpitx -i- -m RF -f "+freq
        FMAMtransmittingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path

        return 'Started transmitting...'
    return 'Still transmitting!'


@app.route('/transAM7000')
def raspi_transAM7000():
    global FMAMtransmittingProcess

    if FMAMtransmittingProcess != None:
        if FMAMtransmittingProcess.poll() != None:
            FMAMtransmittingProcess = None

    if FMAMtransmittingProcess == None:

        ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
        ## user id: id -u
        ## user id = 1000
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 ffmpeg -use_wallclock_as_timestamps 1 -f pulse -i TransmittersSink.monitor -ac 1 -ar 48000 -acodec pcm_s16le -f wav - | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo ./LIBS/rpitx/rpitx -i- -m RF -f 10000"
        FMAMtransmittingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path

        return 'Started transmitting...'
    return 'Still transmitting!'


@app.route('/transAM1600')
def raspi_transAM1600():
    global FMAMtransmittingProcess

    if FMAMtransmittingProcess != None:
        if FMAMtransmittingProcess.poll() != None:
            FMAMtransmittingProcess = None

    if FMAMtransmittingProcess == None:

        ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
        ## user id: id -u
        ## user id = 1000
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 ffmpeg -use_wallclock_as_timestamps 1 -f pulse -i TransmittersSink.monitor -ac 1 -ar 48000 -acodec pcm_s16le -f wav - | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo ./LIBS/rpitx/rpitx -i- -m RF -f 1600"
        FMAMtransmittingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path

        return 'Started transmitting...'
    return 'Still transmitting!'


@app.route('/transStop')
def raspi_transStop():
    global FMAMtransmittingProcess

    if (os.system("ps cax | grep pifmrds") == 0 or os.system("ps cax | grep rpitx") == 0):
        os.system("sudo killall pifmrds")
        os.system("sudo killall rpitx")
        FMAMtransmittingProcess = None
        return 'Stopped transmitting!'
    #if FMAMtransmittingProcess != None:
    #    if FMAMtransmittingProcess.poll() == None:
    #        FMAMtransmittingProcess.terminate()
    #        os.system("sudo killall pifmrds")
    #        os.system("sudo killall rpitx")
    #        #os.system("sudo killall ffmpeg")
    #        return 'Stopped transmitting!'
    #    FMAMtransmittingProcess = None
    return 'Nothing transmitting!'





##########################
# SETTINGS
##########################

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
        os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-default-sink TransmittersSink")
        
        ## check autoplays
        check_autoplay()



if __name__ == '__main__':
    main()
    app.run(debug=True, host='0.0.0.0', port=80)

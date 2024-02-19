from flask import Flask,request
import subprocess
from threading import Thread
import os
from pprint import pprint


app = Flask(__name__)

wifi_device = "wlan0"

@app.route('/')
def index():
    result = subprocess.check_output(["nmcli", "--colors", "no", "-m", "multiline", "--get-value", "SSID", "dev", "wifi", "list", "ifname", wifi_device])
    ssids_list = result.decode().split('\n')
    dropdowndisplay = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>Wifi Control</title>
        </head>
        <body>
            <h1>Wifi Control</h1>
            <form action="/submit" method="post">
                <label for="ssid">Choose a WiFi network:</label>
                <select name="ssid" id="ssid">
        """
    for ssid in ssids_list:
        only_ssid = ssid.removeprefix("SSID:")
        if len(only_ssid) > 0:
            dropdowndisplay += f"""
                    <option value="{only_ssid}">{only_ssid}</option>
            """
    dropdowndisplay += f"""
                </select>
                <p/>
                <label for="password">Password: <input type="password" name="password"/></label>
                <p/>
                <input type="submit" value="Connect">
            </form>
        </body>
        </html>
        """



    dropdowndisplay = ""
    dropdowndisplay += """
    <h1>HistoR</h1><hr>
    <a href="./play">Play</a>
    <a href="./playradio">Play Radio</a>
    <a href="./stop">Stop</a>
    <a href="./volume">Volume</a>
    <a href="./volumeup">Volume Up</a>
    <a href="./volumedown">Volume Down</a>
    
    
    <script>
    // Get the parent DIV, add click listener...
            document.body.addEventListener("click", function(e) {
                // e.target was the clicked element
                if(e.target && e.target.nodeName == "A") {
                    e.preventDefault();
                    //alert(e.target.href.split("/")[3]);
                    runcmd(e.target.href);
                }
            });
            async function runcmd(cmd) {
                  const response = await fetch(cmd);
                  const ret = await response.text();
                  alert(ret);
            }

        </script>
        """
    return dropdowndisplay


@app.route('/submit',methods=['POST'])
def submit():
    if request.method == 'POST':
        print(*list(request.form.keys()), sep = ", ")
        ssid = request.form['ssid']
        password = request.form['password']
        connection_command = ["nmcli", "--colors", "no", "device", "wifi", "connect", ssid, "ifname", wifi_device]
        if length(password) > 0:
          connection_command.append("password")
          connection_command.append(password)
        result = subprocess.run(connection_command, capture_output=True)
        if result.stderr:
            return "Error: failed to connect to wifi network: <i>%s</i>" % result.stderr.decode()
        elif result.stdout:
            return "Success: <i>%s</i>" % result.stdout.decode()
        return "Error: failed to connect."







FMAMtransmittingProcess = None

@app.route('/play')
def raspi_play():
    global FMAMtransmittingProcess
    
    if FMAMtransmittingProcess != None:
        if FMAMtransmittingProcess.poll() != None:
            FMAMtransmittingProcess = None
    
    if FMAMtransmittingProcess == None:
        #play_command = ["nmcli", "--colors", "no", "device", "wifi", "connect", ssid, "ifname", wifi_device]
        #play_command = ["python", "--version"]
        play_command = ["mplayer", "-ao", "alsa:device=hw=0.0", "./MUSIC/Creedence Clearwater Revival - Fortunate Son (Official Music Video).mp3"]
        FMAMtransmittingProcess = subprocess.Popen(play_command, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
        return 'Started Playing...'
    return 'Still playing!'



@app.route('/playradio')
def raspi_playradio():
    global FMAMtransmittingProcess
    
    if FMAMtransmittingProcess != None:
        if FMAMtransmittingProcess.poll() != None:
            FMAMtransmittingProcess = None
    
    if FMAMtransmittingProcess == None:
        #play_command = ["nmcli", "--colors", "no", "device", "wifi", "connect", ssid, "ifname", wifi_device]
        #play_command = ["python", "--version"]
        play_command = "sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 mplayer -ao pulse::0 https://ice5.abradio.cz/hitvysocina128.mp3"
        FMAMtransmittingProcess = subprocess.Popen(play_command, shell = True, cwd=os.path.dirname(os.path.realpath(__file__))) # change working directory to this script path
        return 'Started Playing...'
    return 'Still playing!'


@app.route('/volumeup')
def raspi_volumeup():
    ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
    ## user id: id -u
    ## user id = 1000
    os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-sink-volume 0 +5%")

    output="Volume: "
    try:
        output += str(int(subprocess.check_output("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl list sinks | grep '^[[:space:]]Volume:' | head -n $(( 1 )) | tail -n 1 | sed -e 's,.* \\([0-9][0-9]*\\)%.*,\\1,'", shell = True)))
    except subprocess.CalledProcessError as e:
        output += "pactl stdout output:\n" + repr(e.output)
    output+=" %"
    
    output += '<br>Volume up +5 %!'
    
    return output

@app.route('/volumedown')
def raspi_volumedown():
    ## https://unix.stackexchange.com/questions/457946/pactl-works-in-userspace-not-as-root-on-i3
    ## user id: id -u
    ## user id = 1000
    os.system("sudo -u '#1000' XDG_RUNTIME_DIR=/run/user/1000 pactl set-sink-volume 0 -5%")
    
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
    global FMAMtransmittingProcess
    
    with app.app_context():
    # Your working code here!
        return_code = subprocess.Popen(data['cmd'], cwd=os.path.dirname(os.path.realpath(__file__)))
        if return_code == 0:
            print("Command executed successfully.")
        else:
            print("Command failed with return code", return_code)
        
        FMAMtransmittingProcess = None





@app.route('/stop')
def raspi_stop():
    global FMAMtransmittingProcess
    
    if FMAMtransmittingProcess != None:
        if FMAMtransmittingProcess.poll() == None:
            FMAMtransmittingProcess.terminate()
            os.system("sudo killall mplayer")
            return 'Stopped!'
        FMAMtransmittingProcess = None
    return 'Nothing playing!'

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=80)

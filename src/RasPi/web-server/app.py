from flask import Flask, render_template, render_template_string, request, redirect

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')
    return 'Hello world!'

@app.route('/hello/<name>', strict_slashes=False)
@app.route('/hello/<name>/<page>', strict_slashes=False)
@app.route('/hello/<name>/filter/<filter>', strict_slashes=False)
def hello(name,page="0",filter=""):
    #page = request.args.get('page', default = 1, type = int)
    if filter == "":
        filter = request.args.get('filter', default = '*', type = str)
    return render_template_string('<h1>Hello {{name}}! Page: {{str}}; Filter: {{filter}}</h1>', name=name, str=page, filter=filter)


#https://stackoverflow.com/questions/14023864/flask-url-route-route-all-other-urls-to-some-function
@app.errorhandler(404)
def page_not_found(e):
    # your processing here
    #return 'Hello world! 404', 302
    return redirect("/",code=302)


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=80)

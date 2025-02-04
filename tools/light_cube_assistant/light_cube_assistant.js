var WINDOW_HEIGHT = 600;
var WINDOW_WIDTH  = 600;

var H_MAX = 360;
var S_MAX = 100;
var V_MAX = 100;

var DEFAULT_BACKGROUND = [0, 0, 0];
var DEFAULT_STROKE_COLOR = [250, 0, 100];

var DEBUGVISUALS = false;

var PI  = Math.PI;
var TAU = Math.PI * 2;

var PANEL_NUM_PIXELS_PER_SIDE = 4;
var PIXEL_WIDTH = 50;

////////////////////////

var SAVE_FRAMES = false;
var SAVE_FRAMES_BLACKOUT_THRESHOLD = 1;

var FRAME_LIMITING = false;
var FRAME_PER_SECOND = 60;
if (SAVE_FRAMES)
{
  FRAME_LIMITING = true;
  FRAME_PER_SECOND = 15;
}
var FRAME_PERIOD_MS = 1000 / FRAME_PER_SECOND;

var TOGGLE_DEBUG_ALLOWED = false;
var DEBUG_LINES = false;
var DEBUG_FPS = false;
var DEBUG_PAUSING = false;

var BLACKOUTS_ENABLED = true;
var DEFAULT_NUM_HALF_PERIODS_TIL_BLACKING_OUT = 12;

// UTILS ///////////////
// Takes r,g,b in 0-255
var CG_HMAX = 360;
var CG_SMAX = 100;
var CG_VMAX = 100;
var CG_RMAX = 255;
var CG_GMAX = 255;
var CG_BMAX = 255;

function rgbToHsv(rgb)
{
  r = rgb[0];
  g = rgb[1];
  b = rgb[2];
  var h;
  var s;
  var v;
  var maxColor = Math.max(r, g, b);
  var minColor = Math.min(r, g, b);
  var delta = maxColor - minColor;
  // Calculate hue
  // To simplify the formula, we use 0-6 range.
  if(delta == 0) {
    h = 0;
  }
  else if(r == maxColor) {
    h = (6 + (g - b) / delta) % 6;
  }
  else if(g == maxColor) {
    h = 2 + (b - r) / delta;
  }
  else if(b == maxColor) {
    h = 4 + (r - g) / delta;
  }
  else {
    h = 0;
  }
  // Then adjust the range to be 0-1
  h = h/6;
  // Calculate saturation
  if(maxColor != 0) {
    s = delta / maxColor;
  }
  else {
    s = 0;
  }
  // Calculate value
  v = maxColor / 255;
  return [h * CG_HMAX, s * CG_SMAX, v * CG_VMAX];
}

function hexToRgb(hex) {
  let result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result ? {
    r: parseInt(result[1], 16),
    g: parseInt(result[2], 16),
    b: parseInt(result[3], 16)
  } : null;
}


// GUI GLOBALS /////////
var FILL_COLOR = "#6541d6";


////////////////////////
var DIR_NORTH = 0;
var DIR_WEST  = 1;
var DIR_SOUTH = 2;
var DIR_EAST  = 3;
var DIR_TOP   = 4;
var dirStrings = ["DIR_NORTH", "DIR_WEST", "DIR_SOUTH", "DIR_EAST", "DIR_TOP"];
var panels = [DIR_NORTH, DIR_WEST, DIR_SOUTH, DIR_EAST, DIR_TOP];
var panelChars = ['n', 'w', 's', 'e', 't'];

class Pixel
{
  constructor()
  {
    this.h = 0;
    this.s = 0;
    this.v = 0;
    this.hsv = [this.h, this.s, this.v];
  }

  getHsv()
  {
    return this.hsv;
  }

  setHsv(h, s, v)
  {
    this.hsv = [h,s,v];
  }
}

class Panel
{
  constructor(len, pos, x, y)
  {
    this.len = len;
    this.numPixels = len*len;
    this.pos = pos;
    this.x = x;
    this.y = y;
    this.pixels = Array.from({ length: this.len }, () => Array.from({ length: this.len }, () => new Pixel()));
    for (var x = 0; x < PANEL_NUM_PIXELS_PER_SIDE; x++)
    {
      for (var y = 0; y < PANEL_NUM_PIXELS_PER_SIDE; y++)
      {
        this.pixels[x][y].x = x;
        this.pixels[x][y].y = y;
        this.pixels[x][y].panel = this.pos;
      }
    }
  }

  isMouseIn(x, y)
  {
    if (x > this.x && x < this.x + (this.len * PIXEL_WIDTH) && y > this.y && y < (this.y + (this.len * PIXEL_WIDTH)))
    {
      return true;
    }
  }

  transformCoords(x, y, pos)
  {
    var transX = x;
    var transY = y;
    switch(pos)
    {
      case DIR_NORTH:
        {
          transX = this.len - x - 1;
          transY = y;
          break;
        }
      case DIR_EAST:
        {
          transX = this.len - x - 1;
          transY = this.len - y - 1;
          var tmp = transX;
          transX = transY;
          transY = tmp;
          break;
        }
      case DIR_TOP:
      case DIR_SOUTH:
        {
          transX = x;
          transY = this.len - y - 1;
          break;
        }
      case DIR_WEST:
        {
          transX = x;
          transY = y;
          var tmp = transX;
          transX = transY;
          transY = tmp;
          break;
        }
      default:
        {
        }
    }
    return [transX, transY];
  }

  drawPanel()
  {
    stroke(0, 0, 100);
    rect(this.x, this.y, PIXEL_WIDTH * this.len, PIXEL_WIDTH * this.len);
    for (var x = 0; x < this.len; x++)
    {
      for (var y = 0; y < this.len; y++)
      {
        var transX = x;
        var transY = y;
        var [transX, transY] = this.transformCoords(x, y, this.pos);
        var p = this.pixels[x][y];
        var pCol = p.getHsv();
        fill(pCol[0], pCol[1], pCol[2]);
        rect(this.x + (transX * PIXEL_WIDTH), this.y + (transY * PIXEL_WIDTH), PIXEL_WIDTH, PIXEL_WIDTH);
      }
    }
  }

  getPixel(x, y)
  {
    // console.log(x, y);
    return this.pixels[x][y];
  }

  getPixelFromGlobalCoords(x, y)
  {
    if (!this.isMouseIn(x, y))
    {
      return -1;
    }
    else
    {
      var rawX = int((mouseX - this.x) / PIXEL_WIDTH);
      var rawY = int((mouseY - this.y) / PIXEL_WIDTH);
      var [transX, transY] = this.transformCoords(rawX, rawY, this.pos);
      console.log("PANEL ", dirStrings[this.pos], "LOCAL X", transX, "LOCAL Y", transY);
      return [this.getPixel(transX, transY), transX, transY, this.pos];
    }
  }

	clear()
	{
		for (var i = 0; i < this.len; i++)
		{
			for (var j = 0; j < this.len; j++)
			{
				this.pixels[i][j].setHsv(0, 0, 0);
			}
		}
	}
}

class Canvas 
{
  constructor()
  {
    this.nPanel = new Panel(PANEL_NUM_PIXELS_PER_SIDE, DIR_NORTH, PANEL_NUM_PIXELS_PER_SIDE * PIXEL_WIDTH, 0);
    this.ePanel = new Panel(PANEL_NUM_PIXELS_PER_SIDE, DIR_EAST, PANEL_NUM_PIXELS_PER_SIDE * PIXEL_WIDTH * 2, PANEL_NUM_PIXELS_PER_SIDE * PIXEL_WIDTH);
    this.sPanel = new Panel(PANEL_NUM_PIXELS_PER_SIDE, DIR_SOUTH, PANEL_NUM_PIXELS_PER_SIDE * PIXEL_WIDTH, PANEL_NUM_PIXELS_PER_SIDE * PIXEL_WIDTH * 2);
    this.wPanel = new Panel(PANEL_NUM_PIXELS_PER_SIDE, DIR_WEST, 0, PANEL_NUM_PIXELS_PER_SIDE * PIXEL_WIDTH);
    this.tPanel = new Panel(PANEL_NUM_PIXELS_PER_SIDE, DIR_TOP, PANEL_NUM_PIXELS_PER_SIDE * PIXEL_WIDTH, PANEL_NUM_PIXELS_PER_SIDE * PIXEL_WIDTH);
    this.panels = [this.nPanel, this.ePanel, this.sPanel, this.wPanel, this.tPanel];

    //this.nPanel.pixels[0][0].setHsv(100,100,100);
    //this.ePanel.pixels[0][0].setHsv(100,100,100);
    //this.sPanel.pixels[0][0].setHsv(100,100,100);
    //this.wPanel.pixels[0][0].setHsv(100,100,100);
    //this.tPanel.pixels[0][0].setHsv(100,100,100);
  }

  getPanel(dir)
  {
    return this.panels[dir];
  }

  updateCanvas()
  {
  }

  drawCanvas()
  {
    for (var i = 0; i < this.panels.length; i++)
    {
      this.panels[i].drawPanel();
    }
  }

  drawDebugPanel()
  {
  }

  saveFrame()
  {
  }

	clear()
	{
		for (var i = 0; i < this.panels.length; i++)
		{
			this.panels[i].clear();
		}
	}
}

////////////////////////

function mouseMoved()
{
}

function mouseWheel()
{
}

var lastPanel = 0xff;
var lastTransX = 0xff;
var lastTransY = 0xff;
function mouseClicked()
{
  // console.log("MOUSE CLICKED", mouseX, mouseY);

  // find which panel
  var pan = 0xff;
  for (var i = 0; i < this.myCanvas.panels.length; i++)
  {
    if (myCanvas.panels[i].isMouseIn(mouseX, mouseY))
    {
      pan = i;
      break;
    }
  }

  if (pan == 0xff)
  {
    return;
  }

  // Find which pixel (raw coords)
  var p, transX, transY;
  [p, transX, transY, pos] = myCanvas.panels[pan].getPixelFromGlobalCoords(mouseX, mouseY);

  if (pan == lastPanel && transX == lastTransX && transY == lastTransY)
  {
    return;
  }
  lastPanel = pan;
  lastTransX = transX;
  lastTransY = transY;

  // console.log(p);
  // console.log(hexToRgb(FILL_COLOR));
  var rgb = hexToRgb(FILL_COLOR);
  var hsv = rgbToHsv([rgb.r, rgb.g, rgb.b]);
  p.setHsv(hsv);

  //
	console.log("PAN", pan, pos); //
	if (currentAnimation == "gameoflife")
	{
		var cmd = "anim alive " + panelChars[pos] + " " + transX + " " + transY ; 
	}
	else
	{
		var cmd = "aled set " + panelChars[pan] + " " + transX + " " + transY + " " + rgb.r + " " + rgb.g + " " + rgb.b;
	// var cmd = "aled nei " + panelChars[pos] + " " + transX + " " + transY ; 
	}
  writeToStream(cmd);
}

function mouseDragged()
{
  mouseClicked();
}

function keyPressed()
{
  // console.log("KEY PRESSED", key);
  if (key == ' ')
  {
    myCanvas.paused = false;
  }
}

function keyReleased()
{
  // console.log("KEY RELEASED", key);
  if (key == ' ')
  {
    myCanvas.paused = true;
  }
  if (key == 'd' && TOGGLE_DEBUG_ALLOWED)
  {
    background(0, 0, 0);
    DEBUG_LINES = !DEBUG_LINES;
    DEBUG_FPS = !DEBUG_FPS;
  }
}

function reset()
{

}

////////////////////////
// EDITABLE VALUE LIST / CONFIG STUFF

const fixedPts = 3;
const intChange = 1;
const floatChange = 0.001;
var floatChangeStr = floatChange.toString();
var intChangeStr = intChange.toString();
var currentAnimation = "canvas";

function typeIsFloatOrDouble(type)
{
	// types 
	// UINT8_T,
	// UINT16_T,
	// UINT32_T,
	// DOUBLE,
	// FLOAT,
	// BOOLEAN
	var isFloat = false;
	if (type == 3 || type == 4)
	{
		isFloat = true;
	}

	if (type < 0 || type > 5)
	{
		console.log(`Bad type! ${type}`);
	}
	return isFloat;
}

function confRefresh()
{
	writeToStream("anim getval");
}

function confRefreshByIdx(idx)
{
	writeToStream(`anim getval ${idx}`);
}

function confRefreshByName(name)
{
	writeToStream(`anim getval ${name}`);
}

function confChanged(domElement)
{
	var domType = domElement.type;
	var id = domElement.id;
	var valName = id.split("_")[0];
	var currVal = Number(document.getElementById(`${valName}_currVal`).innerHTML);
	var currValDom = document.getElementById(`${valName}_currVal`);
	var ll = Number(document.getElementById(`${valName}_ll`).innerHTML);
	var ul = Number(document.getElementById(`${valName}_ul`).innerHTML);
	var valType = Number(document.getElementById(`${valName}_type`).innerHTML);
	var isFloat = typeIsFloatOrDouble(valType);
	var cmd = "";
	switch(domType)
	{
		case "range":
		{
			var newVal = Number(domElement.value);
			console.log(`${valName} slider changed from ${currVal} to ${newVal}`);
			newVal = (isFloat ? newVal.toFixed(fixedPts) : newVal);
			cmd = `anim setval ${valName} ${newVal}`;
			currValDom.innerHTML = Number(newVal);
			writeToStream(cmd);
			break;
		}
		case "button":
		{
			var isPlus = (id.split("_")[1]) == "plus";
			var valDiff = (isFloat ? floatChange : intChange) * (isPlus ? 1 : -1);
			var newVal = currVal + valDiff;
			newVal = (isFloat ? newVal.toFixed(fixedPts) : newVal);
			console.log(`${valName} p/m button press ${isPlus} newVal ${newVal}`);
			cmd = `anim setval ${valName} ${newVal}`;
			currValDom.innerHTML = Number(newVal);
			writeToStream(cmd);
			confRefresh();
			break;
		}
		default:
		{
			console.log("confChange bad type!");
			return;
		}
	}
}

function newAnimConfPanel(name)
{
	var tableDom = document.getElementById("animConfPanel");
	tableDom.innerHTML = "";
	var row = tableDom.insertRow(0);
	row.align = "left";
	row.innerHTML = `<td></td><td>.....val_name.....</td><td>.type.</td><td>.......val.......</td><td>ll</td><td>ul</td>`;
	var nameDom = document.getElementById("animConfName");
	nameDom.innerHTML = `Animation Config: ${name}`;
}

function addToAnimConfPanel(idx, name, type, currVal, ll, ul)
{	
	var tableDom = document.getElementById("animConfPanel");
	var currTableLen = tableDom.rows.length;
	var inputDom = "";
	var isFloatDouble = typeIsFloatOrDouble(type)
	if (!isFloatDouble) // int
	{
		inputDom = `
			<td>
				<input type="range" min=${ll} max=${ul} value=${currVal} class="slider" id="${name}_confSlider" 
					oninput="confChanged(this)" 
					onmouseup="confRefresh()"
				>
			</td>
			<td>
				<input type="button" value="-" id="${name}_minus" 
					onclick="confChanged(this)"
				>
			</input>
			</td>
			<td>
				<input type="button" value="+" id="${name}_plus" 
					onclick="confChanged(this)"
				>
			</input>
			</td>`;
	}
	else // float or double
	{
		inputDom = `
			<td>
				<input type="range" min=${ll} max=${ul} value=${currVal} class="slider" id="${name}_confSlider" 
					oninput="confChanged(this)"
					onmouseup="confRefresh()"
					step="${floatChangeStr}"
				>
			</td>
			<td>
				<input type="button" value="-" id="${name}_minus" 
					onclick="confChanged(this)"
				>
			</input>
			</td>
			<td>
				<input type="button" value="+" id="${name}_plus" 
					onclick="confChanged(this)"
				>
			</input>
			</td>`;
	}
	var row = tableDom.insertRow(currTableLen);
	row.align = "left";
	row.id = `${name}_row`;
	row.innerHTML = `<td>${idx}</td><td>${name}</td><td id="${name}_type">${type}</td><td id="${name}_currVal">${currVal}</td><td id="${name}_ll">${ll}</td><td id="${name}_ul">${ul}</td>${inputDom}`;
	
}

////////////////////////
// SERIAL STUFF

function parseSerialDataLine(line)
{
	console.log(`[*] line len ${line.length} ${line}`);
	lines = line.split("\n");
	for (var lid = 0; lid < lines.length; lid++)
	{
		lineArr = lines[lid].split(" ");
		if (lines[lid].includes("Editable list name")) // new config list
		{
			// Reset our config list
			var name = lineArr[3];
			var len = lineArr[5];
			console.log(`Editable List: ${name} len: ${len}`);
			newAnimConfPanel(name);
			currentAnimation = name;
		}
		else if (lines[lid].includes("Editable val")) // single config line
		{ 
			// Append to our config
			var valIdx = Number(lineArr[0]);
			var valName = lineArr[3];
			var valType = Number(lineArr[5]);
			var val = Number(lineArr[7]);
			var ll = Number(lineArr[9]);
			var ul = Number(lineArr[11]);
			console.log(`Editable Value: ${valIdx} ${valName} type:${valType} val:${val} ll:${ll} ul:${ul}`);
			addToAnimConfPanel(valIdx, valName, valType, val, ll, ul);
		}
		else if (lines[lid].includes("Immediately")) // animation changed. poll for configs 
		{
			hookButtonPressed();	
		}
	}
}

////////////////////////
// HTML GUI

function clearButtonPressed()
{
	var cmd = "aled clear";
  writeToStream(cmd);
	for (var i = 0; i < myCanvas.panels.length; i++)
	{
		myCanvas.panels[i].clear();
	}
	myCanvas.clear();
}

function autoButtonPressed()
{
	var cmd = "anim auto";
  writeToStream(cmd);
}

function nextAnimButtonPressed()
{
	var cmd = "anim next";
  writeToStream(cmd);
}

function resetButtonPressed(boot)
{
	var cmd = "reboot";
	// if (boot) // This was for the rp2040 port
	// {
	// 	cmd += "boot";
	// }
	// else
	// {
	// 	cmd += "sw";
	// }
	writeToStream(cmd);
}

function connectButtonPressed()
{
	connect();
}

function hookButtonPressed()
{
	readLoopWrapper(parseSerialDataLine);
	console.log("Hooked!");
	var cmd = "anim getval";
	writeToStream(cmd);
}

function animationSelectChanged()
{
	var animation = event.target.value;
	var cmd = "anim set " + animation;
	writeToStream(cmd);
	myCanvas.clear();
}

////////////////////////
myCanvas = new Canvas();
p5jsCanvas = undefined;
gui = undefined;

function setup()
{
  p5jsCanvas = createCanvas(WINDOW_WIDTH, WINDOW_HEIGHT);
  colorMode(HSB, H_MAX, S_MAX, V_MAX);
  background(DEFAULT_BACKGROUND);
  textSize(12);
  smooth(8);

  gui1 = createGui();
  sliderRange(0, 100, 1);
  gui1.addGlobals("FILL_COLOR");
}

var lastFrameTs = 0;
var fps = 0;
var timeSinceLastFrameMsMs = 0;
function draw()
{
  var frameTs = Date.now();
  if (lastFrameTs != 0)
  {
    timeSinceLastFrameMsMs = frameTs - lastFrameTs;
    fps = int(1000 / timeSinceLastFrameMsMs);

    if (FRAME_LIMITING && timeSinceLastFrameMsMs < FRAME_PERIOD_MS)
    {
      return;
    }
  }
  lastFrameTs = frameTs;
  myCanvas.updateCanvas();
  myCanvas.drawCanvas();
  myCanvas.drawDebugPanel();
}

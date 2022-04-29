
var cvtbuf   = new ArrayBuffer(8);
var cvt8     = new Uint8Array(cvtbuf);
var cvtflt64 = new Float64Array(cvtbuf);
var cvtint32 = new Int32Array(cvtbuf);
var membuf   = new ArrayBuffer(1024*128);
var mem8     = new Uint8Array(membuf);
var memint32 = new Int32Array(membuf);
var memflt64 = new Float64Array(membuf);

function Memcpy(dst, src, count)
{
	for(i=0;i<count;i++)
	{
		mem8[dst++]=mem8[src++]
	}
}

function GetInt(addr)
{
	return memint32[addr>>2];
}

function PutInt(addr, value)
{
	memint32[addr>>2]=value;
}

function GetFloat(addr)
{
	addr|=0;
	cvtint32[0]=memint32[addr>>2];
	cvtint32[1]=memint32[(addr>>2)+1];
	return cvtflt64[0];
}

function GetFloat4(addr)
{
	addr|=0;
	cvtint32[0]=memint32[addr];
	cvtint32[1]=memint32[addr+1];
	return cvtflt64[0];
}

function PutFloat(addr, value)
{
	addr|=0;
	cvtflt64[0]=value;
	memint32[addr>>2]    =cvtint32[0];
	memint32[(addr>>2)+1]=cvtint32[1];
}

function PutFloat4(addr, value)
{
	addr|=0;
	cvtflt64[0]=value;
	memint32[addr]  =cvtint32[0];
	memint32[addr+1]=cvtint32[1];
}

function GetChar(addr)
{
	return mem8[addr];
}

function PutChar(addr, value)
{
	mem8[addr]=value;
}

function GetString(addr)
{
	var s="";
	var i=0;
	while(true)
	{
		var c=GetChar(addr+i);
		if(c==0)
			break;
		if(i>10240)
			break;
		s+=String.fromCharCode(c);
		i++;
	}
	return s;
}


function SC_sqrt()
{
	var argOffset=16;
	var arg=GetFloat(reg_sp-argOffset);
	PutFloat(reg_sp-8, Math.sqrt(arg));
}

function SC_sin()
{
	var argOffset=16;
	var arg=GetFloat(reg_sp-argOffset);
	PutFloat(reg_sp-8, Math.sin(arg));
}

function SC_cos()
{
	var argOffset=16;
	var arg=GetFloat(reg_sp-argOffset);
	PutFloat(reg_sp-8, Math.cos(arg));
}

function SC_Time()
{
	PutFloat(reg_sp-8, performance.now()/1000);
}

function Format(format, offset)
{
	while(true)
	{
		var percentpos=format.search("%")
		if(percentpos<0)
			break;
		if(format[percentpos+1]=="i")
		{
			offset+=4;
			format=format.replace("%i",GetInt(reg_sp-offset))
		}
		else if(format[percentpos+1]=="f")
		{
			offset+=8;
			format=format.replace("%f",GetFloat(reg_sp-offset))
		}
		else if(format[percentpos+1]=="s")
		{
			offset+=4;
			var s=GetString(GetInt(reg_sp-offset));
			format=format.replace("%s",s)
		}
		else if(format[percentpos+1]=="c")
		{
			offset+=4;
			var s=GetInt(reg_sp-offset);
			format=format.replace("%c",String.fromCharCode(s))
		}
		else
		{
			format=format.replace("%","")
		}
	}
	return format
}


function SC_snprintf()
{
	//	6: int snprintf(char*,int,char*,...);// #6
	var dst=GetInt(reg_sp-8)
	var count=GetInt(reg_sp-12)
	var format=GetString(GetInt(reg_sp-16))
	format=Format(format, 16)
	for(var i=0;i<count;i++)
	{
		if(i<format.length)
		{
			PutChar(dst+i,format.charCodeAt(i));
		}
		else
		{
			PutChar(dst+i,0);
			break;
		}
	}
	PutInt(reg_sp-4, count)
}


function SC_printf()
{
	var argOffset=8;
	var format=GetString(GetInt(reg_sp-argOffset));
	format=Format(format, 8)
	if(gui)
		out.innerText+=format
	console.log(format)
	printed++
}

function SC_puts()
{
	var argOffset=8;
	var format=GetString(GetInt(reg_sp-argOffset))+"\n";
	if(gui)
		out.innerText+=format
	console.log(format)
	printed++
}

function SC_malloc()
{
	var count=GetInt(reg_sp-8);
	var oldMem8=mem8
	addr=membuf.byteLength;
	membuf   = new ArrayBuffer(1024*128+(count/8+1)*8);
	mem8     = new Uint8Array(membuf);
	memint32 = new Int32Array(membuf);
	memflt64 = new Float64Array(membuf);
	mem8.set(oldMem8);
	PutInt(reg_sp-4,addr)
}

function SC_memcpy()
{
	var toIdx=GetInt(reg_sp-8);
	var fromIdx=GetInt(reg_sp-12);
	var count=GetInt(reg_sp-16);
	mem8.copyWithin(toIdx,fromIdx,fromIdx+count)
}

function SC_CreateRT(){}

function SC_BitBlt(){}

var g_col_r;
var g_col_g;
var g_col_b;
var g_col_a;

function SC_rgba32()
{
	var col=GetInt(reg_sp-4);
	g_col_r=((col>>16)&0xff)/255.;
	g_col_g=((col>>8 )&0xff)/255.;
	g_col_b=((col    )&0xff)/255.;
	g_col_a=((col>>24)&0xff)/255.;
}

function SC_rgb()
{
	g_col_r=GetFloat(reg_sp-24);
	g_col_g=GetFloat(reg_sp-16);
	g_col_b=GetFloat(reg_sp-8);
}

function SC_rgba()
{
	g_col_r=GetFloat(reg_sp-24);
	g_col_g=GetFloat(reg_sp-16);
	g_col_b=GetFloat(reg_sp-8);
	g_col_a=GetFloat(reg_sp-32);
}

function SC_gray()
{
	var gr=GetFloat(reg_sp-8);
	g_col_r=gr;
	g_col_g=gr;
	g_col_b=gr;
}

function SC_hairline()
{
	var x0=GetFloat(reg_sp-8);
	var y0=GetFloat(reg_sp-16);
	var x1=GetFloat(reg_sp-24);
	var y1=GetFloat(reg_sp-32);
	ctx.beginPath();
	ctx.moveTo(x0+.5,y0+.5);
	ctx.lineTo(x1+.5,y1+.5);
	ctx.strokeStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.globalAlpha=g_col_a
	ctx.lineWidth=1
	ctx.stroke();
}

function SC_FillRT()
{
	ctx.fillStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.globalAlpha=1
	ctx.fillRect(0, 0, screen.width, screen.height);
}

function SC_miterlim()
{
}

const useStringPath=false

var g_path

if(useStringPath)g_path=""
else g_path=new Path2D()

function SC_Rect()
{
	var x0 =GetInt(reg_sp-4);
	var y0 =GetInt(reg_sp-8);
	var x1 =GetInt(reg_sp-12);
	var y1 =GetInt(reg_sp-16);
	var col=GetInt(reg_sp-20);
	g_col_r=((col>>16)&0xff)/255.;
	g_col_g=((col>>8 )&0xff)/255.;
	g_col_b=((col    )&0xff)/255.;
	g_col_a=((col>>24)&0xff)/255.;
	ctx.fillStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.globalAlpha=g_col_a
	ctx.fillRect(x0, y0, x1, y1);
}

function SC_GetMouseState()
{
	var pmx=GetInt(reg_sp-4);
	var pmy=GetInt(reg_sp-8);
	var pmb=GetInt(reg_sp-12);
	memint32[pmx>>2]=SC_mx;
	memint32[pmy>>2]=SC_my;
	memint32[pmb>>2]=SC_mb;
}
function SC_KeyPressed()
{
	let keyCode=memint32[(reg_sp-8)>>2]
	memint32[(reg_sp-4)>>2]=keysPressed.has(keyCode);
}
function SC_SetPersistentFloat(){};
function SC_GetPersistentFloat(){};
function SC_pow()
{
	PutFloat(reg_sp-8, Math.pow(GetFloat(reg_sp-16),GetFloat(reg_sp-24)));
};

function SC_fillrect()
{
	var x0 =GetInt(reg_sp-4);
	var y0 =GetInt(reg_sp-8);
	var x1 =GetInt(reg_sp-12);
	var y1 =GetInt(reg_sp-16);
	ctx.fillStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.globalAlpha=g_col_a
	ctx.fillRect(x0, y0, x1, y1);
}

function SC_lineH()
{
	var x0 =GetInt(reg_sp-4);
	var y0 =GetInt(reg_sp-8);
	var l =GetInt(reg_sp-12);
	var col =GetInt(reg_sp-16);
	g_col_r=((col>>16)&0xff)/255.;
	g_col_g=((col>>8 )&0xff)/255.;
	g_col_b=((col    )&0xff)/255.;
	g_col_a=((col>>24)&0xff)/255.;
	ctx.globalAlpha=g_col_a
	ctx.fillStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.fillRect(x0, y0, l, 1);
}

function SC_lineV()
{
	var x0 =GetInt(reg_sp-4);
	var y0 =GetInt(reg_sp-8);
	var l =GetInt(reg_sp-12);
	var col =GetInt(reg_sp-16);
	g_col_r=((col>>16)&0xff)/255.;
	g_col_g=((col>>8 )&0xff)/255.;
	g_col_b=((col    )&0xff)/255.;
	g_col_a=((col>>24)&0xff)/255.;
	ctx.globalAlpha=g_col_a
	ctx.fillStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.fillRect(x0, y0, 1, l);
}

function SC_SetPresentWait(){}

function SC_clear()
{
	if(useStringPath)g_path=""
	else g_path=new Path2D()
	ctx.beginPath()
}

var SC_gcpx
var SC_gcpy

function SC_M()
{
	SC_gcpx=GetFloat(reg_sp-8)
	SC_gcpy=GetFloat(reg_sp-16)
	if(useStringPath)
	{
		g_path+="M "+SC_gcpx+" "+(SC_gcpy-0.00001)
		g_path+="L "+SC_gcpx+" "+SC_gcpy
	}
	else
	{
		g_path.moveTo(SC_gcpx,SC_gcpy-0.000001)
		g_path.lineTo(SC_gcpx,SC_gcpy)
	}
}

function SC_m()
{
	SC_gcpx+=GetFloat(reg_sp-8)
	SC_gcpy+=GetFloat(reg_sp-16)
	if(useStringPath)
	{
		g_path+="M "+SC_gcpx+" "+SC_gcpy
		g_path+="l 0 0.000001"
	}
	else
	{
		g_path.moveTo(SC_gcpx,SC_gcpy-0.000001)
		g_path.lineTo(SC_gcpx,SC_gcpy)
	}
}

function SC_L()
{
	SC_gcpx=GetFloat(reg_sp-8)
	SC_gcpy=GetFloat(reg_sp-16)
	if(useStringPath)g_path+="L "+SC_gcpx+" "+SC_gcpy
	else g_path.lineTo(SC_gcpx, SC_gcpy);
}

function SC_l()
{
	SC_gcpx+=GetFloat(reg_sp-8)
	SC_gcpy+=GetFloat(reg_sp-16)
	if(useStringPath)g_path+="L "+SC_gcpx+" "+SC_gcpy
	else g_path.lineTo(SC_gcpx, SC_gcpy);
}

function SC_C()
{
	//void Graph::C(float,float,float,float,float,float);// #49
	var x0=GetFloat(reg_sp-8)
	var y0=GetFloat(reg_sp-16)
	var x1=GetFloat(reg_sp-24)
	var y1=GetFloat(reg_sp-32)
	var x=GetFloat(reg_sp-40)
	var y=GetFloat(reg_sp-48)
	if(useStringPath)g_path+="C "+x0+" "+y0+" "+x1+" "+y1+" "+x+" "+y
	else g_path.bezierCurveTo(x0,y0,x1,y1,x,y);
	SC_gcpx=x
	SC_gcpy=y
}

function SC_c()
{
	//void Graph::C(float,float,float,float,float,float);// #49
	var x0=GetFloat(reg_sp-8)+SC_gcpx
	var y0=GetFloat(reg_sp-16)+SC_gcpy
	var x1=GetFloat(reg_sp-24)+SC_gcpx
	var y1=GetFloat(reg_sp-32)+SC_gcpy
	var x=GetFloat(reg_sp-40)+SC_gcpx
	var y=GetFloat(reg_sp-48)+SC_gcpy
	if(useStringPath)g_path+="C "+x0+" "+y0+" "+x1+" "+y1+" "+x+" "+y
	else g_path.bezierCurveTo(x0,y0,x1,y1,x,y);
	SC_gcpx=x
	SC_gcpy=y
}

function SC_a()
{
	//void Graph::a(float,float,float,int,int,float,float);// #54
	var rx   =GetFloat(reg_sp-8)
	var ry   =GetFloat(reg_sp-16)
	var a    =GetFloat(reg_sp-24)
	var sweep=GetFloat(reg_sp-28)|0
	var large=GetFloat(reg_sp-32)|0
	var dx   =GetFloat(reg_sp-40)
	var dy   =GetFloat(reg_sp-48)
	var path = 
	//'M '+SC_gcpx+' '+SC_gcpy+' '+
	'a'
	+' '+rx
	+' '+ry
	+' '+a
	+' '+(sweep)
	+' '+(1-large)
	+' '+(dx)
	+' '+(dy)
	//g_path.addPath(new Path2D(path))
	SC_gcpx+=dx
	SC_gcpy+=dy

	if(useStringPath)g_path+=path
	else g_path.lineTo(SC_gcpx,SC_gcpy);
}

function SC_Circle()
{
	var x=GetFloat(reg_sp-8)
	var y=GetFloat(reg_sp-16)
	var r=GetFloat(reg_sp-24)
	var w=GetFloat(reg_sp-32)
	var a=GetFloat(reg_sp-40)
	var col=GetInt(reg_sp-44)
	g_col_r=((col>>16)&0xff)/255.;
	g_col_g=((col>>8 )&0xff)/255.;
	g_col_b=((col    )&0xff)/255.;
	g_col_a=((col>>24)&0xff)/255.;
	path=new Path2D
	path.moveTo(x, y);
	path.lineTo(x+0.0001, y);
	ctx.strokeStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.lineCap="round"
	ctx.lineJoin="round"
	ctx.globalAlpha=g_col_a/4.
	for(i=0;i<4;i++)
	{
		ctx.lineWidth=a*4*i/4
		ctx.stroke(path)
	}
}

function SC_close()
{
	if(useStringPath)g_path+=" z "
	else g_path.closePath(SC_gcpx,SC_gcpy);
}

function SC_fin(){}

function SC_alpha()
{
	g_col_a=GetFloat(reg_sp-8)
}

function SC_graddef(){}
function SC_gradend(){}
function SC_gradmethod(){}
function SC_gradstop(){}
function SC_graduse(){}
function SC_gradtype(){}
function SC_g_t(){}

var g_width1=1
var g_width2=1

function SC_width()
{
	g_width1=GetFloat(reg_sp-8)
	g_width2=GetFloat(reg_sp-16)
}

function SC_fill1()
{
	ctx.fillStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.globalAlpha=g_col_a
	if(useStringPath)ctx.fill(new Path2D(g_path),"nonzero");
	else ctx.fill(g_path,"nonzero");
	
}

function SC_fill2()
{
	ctx.fillStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.globalAlpha=g_col_a
	if(useStringPath)ctx.fill(new Path2D(g_path),"evenodd");
	else ctx.fill(g_path,"evenodd");
}

function SC_stroke()
{
	ctx.strokeStyle='rgb('+g_col_r*255+','+g_col_g*255+','+g_col_b*255+')'
	ctx.lineCap="round"
	ctx.lineJoin="round"
	var width2=g_width1-.5
	var walpha=1.;
	var width1=width2-Math.abs(width2/g_width2)
	if(width1<0)
	{
		walpha=1./-width1;
		width1=0;
	}
	if(width1>width2)
	{
		var tmp=width1
		width1=width2
		width2=tmp
	}

	var dw=width2-width1
	var d=Math.trunc(dw*1.25+.5);
	if(useStringPath)var path=new Path2D(g_path)
	
	if(d<2)
	{
		ctx.globalAlpha=g_col_a
		ctx.lineWidth=width2*2
		//ctx.strokeStyle='red';
		if(useStringPath)ctx.stroke(path)
		else ctx.stroke(g_path)
	}
	else
	{
		if(d>16)d=16
		var a=0;
		var v=a;
		for(i=0;i<d;i++)
		{
			v=(((i+.5)*g_col_a)/(d-.5)-a)/(1.-a);
			ctx.globalAlpha=v;
			a=a+(1.-a)*v;
			ctx.lineWidth=(width2-(width2-width1)*(i/(d-1.)))*2
			if(useStringPath)ctx.stroke(path)
			else ctx.stroke(g_path)
		}
	}
}

let g_t0x=0
let g_t0y=0
let g_tXx=1
let g_tXy=0
let g_tYx=0
let g_tYy=1


function SC_t_0()
{
	g_t0x=GetFloat(reg_sp-8)
	g_t0y=GetFloat(reg_sp-16)
	ctx.setTransform(g_tXx, g_tXy, g_tYx, g_tYy, g_t0x, g_t0y);
}
function SC_t_x()
{
	g_tXx=GetFloat(reg_sp-8)
	g_tXy=GetFloat(reg_sp-16)
	ctx.setTransform(g_tXx, g_tXy, g_tYx, g_tYy, g_t0x, g_t0y);
}
function SC_t_y()
{
	g_tYx=GetFloat(reg_sp-8)
	g_tYy=GetFloat(reg_sp-16)
	ctx.setTransform(g_tXx, g_tXy, g_tYx, g_tYy, g_t0x, g_t0y);
}

function SC_g_0()
{

}
function SC_g_t()
{

}
function SC_g_x()
{

}
function SC_g_y()
{

}

function SC_DrawText(s)
{
	for(var i=0;i<s.length;i++)
		SC_Glyph(s.charCodeAt(i))
}

function SC_gtext()
{
	var stext=GetString(GetInt(reg_sp-8))
	SC_DrawText(stext)
}

function SC_glyph()
{
	var c=GetInt(reg_sp-8)
	SC_Glyph(c)
}

function SC_stext()
{
	var stext=GetString(GetInt(reg_sp-8))
	var x=GetInt(reg_sp-12)
	var y=GetInt(reg_sp-16)+10
	var col=GetInt(reg_sp-20)
	var c_r=((col>>16)&0xff);
	var c_g=((col>>8 )&0xff);
	var c_b=((col    )&0xff);
	var c_a=((col>>24)&0xff);

	if(stext.length==0)
		return
//	var img = ctx.getImageData(x,y,screen.width, 12);
	ctx.fillStyle='rgb('+c_r+','+c_g+','+c_b+')'
	ctx.globalAlpha=c_a/255.;
	//ctx.font = '10px monospace';
	ctx.fillText(stext, x, y);
	return
	var orgx=x
	var orgy=y
	//x=0
	//y=0
	for(var k=0;k<stext.length;k++)
	{
		var c=stext.charCodeAt(k)-32
		x+=6
		if(c>=256)
			continue;
		for(var i=0;i<12;i++)
		{
			for(var j=0;j<6;j++)
			{
				if(font6x12[c*6*12+i*6+j]!==" ")
				{
					// img.data[(screen.width*(y+i)+x+j)*4]  =c_r
					// img.data[(screen.width*(y+i)+x+j)*4+1]=c_g
					// img.data[(screen.width*(y+i)+x+j)*4+2]=c_b
					// img.data[(screen.width*(y+i)+x+j)*4+3]=c_a
					ctx.fillRect(x+j,y+i,1,1);
				}
			}
		}
	}
	//ctx.putImageData(img,orgx,orgy)
}

function SC_GetScincEvent()
{
	if(events.length>0)
	{
		let addr=GetInt(reg_sp-8);
		PutInt(addr,events[0].type)
		PutInt(addr+4,events[0]._0)
		PutInt(addr+8,events[0]._1)
		PutInt(addr+12,events[0]._2)
		PutInt(reg_sp-4,events.length)
		events.shift()
	}
	else
	{
		PutInt(reg_sp-4,0)
	}
}

function SC_GetKeyEvent()
{
	if(keyEvents.length>0)
	{
		PutInt(GetInt(reg_sp-8),keyEvents[0][0])
		PutInt(GetInt(reg_sp-12),keyEvents[0][1])
		keyEvents.shift()
		PutInt(reg_sp-4,1)
	}
	else
	{
		PutInt(reg_sp-4,0)
	}
}

function SC_CountKeyEvents()
{
	PutInt(reg_sp-4,keyEvents.length)
}

// Sound

let audioContext
let scincAudioQueue

function StartSound()
{

	audioContext = new AudioContext()
	scincAudioQueue=[]

	async function CreateAudio()
	{
		let newAudioNode
		try
		{
			console.log("adding...")
			await audioContext.audioWorklet.addModule("SC_Audio.js");
			console.log("OK")
			newAudioNode = new AudioWorkletNode(audioContext, "ScincAudio", 
			{
				numberOfInputs:0,
				numberOfOutputs:1,
				outputChannelCount:[2],
			})
		}
		catch(e)
		{
			try
			{
				console.log("adding... once more")
				await audioContext.audioWorklet.addModule("SC_Audio.js");
				console.log("OK")
				newAudioNode = new AudioWorkletNode(audioContext, "ScincAudio");
			}
			catch(e)
			{
				console.log('** Error: Unable to create worklet node: ${e}');
			}
		}
		newAudioNode.scbuf=scincAudioBuf
		ready=true
		return newAudioNode
	}

	let processorNode;

	health=0

	silenceBuf=[]
	for(let i=0;i<256;i++)silenceBuf.push(0)

	async function StartAudio()
	{
		processorNode=await CreateAudio()
		processorNode.port.onmessage=function(e)
		{
			if(scincAudioQueue.length)
			{
				processorNode.port.postMessage(scincAudioQueue[0])
				scincAudioQueue.shift()
			}
			else
			{
				processorNode.port.postMessage(silenceBuf)
			}
	}
		processorNode.connect(audioContext.destination)
		console.log("Resuming")
		await audioContext.resume()
		console.log("OK")
	}

	StartAudio()

}

var sounds=[]

function SC_snd_add()
{
	let len=GetInt(reg_sp-8)
	let newBuf=audioContext.createBuffer(2,len,48000)
	sounds.push(newBuf)
	PutInt(reg_sp-4,sounds.length-1)
}
function SC_snd_data()
{
	let index=GetInt(reg_sp-4)
	let offset=GetInt(reg_sp-8)
	let l=GetFloat(reg_sp-16)
	let r=GetFloat(reg_sp-24)
	sounds[index].getChannelData(0)[offset]=l
	sounds[index].getChannelData(1)[offset]=r
}
function SC_snd_play()
{
	let index=GetInt(reg_sp-4)
	var source=audioContext.createBufferSource()
	source.buffer=sounds[index]
	source.connect(audioContext.destination)
	source.start()
}

function SC_snd_bufhealth()
{
	PutInt(reg_sp-4,scincAudioQueue.length*128)
}

var scincAudioBuf=new Float32Array(256)
var sample=0

function SC_snd_out()
{
	scincAudioBuf[sample++]=GetFloat(reg_sp-8)
	scincAudioBuf[sample++]=GetFloat(reg_sp-16)
	if(sample>=256)
	{
		scincAudioQueue.push(scincAudioBuf)
		scincAudioBuf=new Float32Array(256)
		sample=0
	}
}

function SC_snd_out_buf()
{
	var addr=GetInt(reg_sp-4)/8
	var count=GetInt(reg_sp-8)
	for(let i=0;i<count*2;i+=2)
	{
		scincAudioBuf[sample++]=memflt64[addr+i]
		scincAudioBuf[sample++]=memflt64[addr+i+1]
		if(sample>=256)
		{
			scincAudioQueue.push(scincAudioBuf)
			scincAudioBuf=new Float32Array(256)
			sample=0
			}
	}
}

function SC_SetPersistentInt(){}

function SC_GetPersistentInt(){}

function SC_t_t(){}

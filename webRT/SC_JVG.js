let Scinc_SVGs={}
let Scinc_Anims={}
let Scinc_SVGsByIndex=[]
let Scinc_AnimsByIndex=[]
let promises=[]

function SC_SVGLoad()
{
	var svgname=GetString(GetInt(reg_sp-8))
	if(svgname.endsWith(".svg"))
		svgname=svgname.replace(/svg$/,"jvg")
	console.log("SVG loaded:"+svgname)
	let svgIndex=Scinc_SVGsByIndex.length
	Scinc_SVGsByIndex.push(null)
	SC_LoadRes(svgname,svgname,Scinc_SVGs,Scinc_SVGsByIndex,svgIndex,jvg=>SC_PrepareJVG(ctx,jvg))
	PutInt(reg_sp-4,svgIndex)
}

function SC_SVGLoadAnimation()
{
	var animname=GetString(GetInt(reg_sp-12))
	if(animname.endsWith(".ami"))
		animname=animname.replace(/ami$/,"jami")
	console.log("Anim loaded:"+animname)
	let animIndex=Scinc_AnimsByIndex.length
	Scinc_AnimsByIndex.push(null)
	SC_LoadRes(animname,animname,Scinc_Anims,Scinc_AnimsByIndex,animIndex,anim=>SC_PrepareAnim(anim))
	PutInt(reg_sp-4,animIndex)
}

function SC_SVGSetTransform()
{
	var t0x=GetFloat(reg_sp- 8)
	var t0y=GetFloat(reg_sp-16)
	var txx=GetFloat(reg_sp-24)
	var txy=GetFloat(reg_sp-32)
	var tyx=GetFloat(reg_sp-40)
	var tyy=GetFloat(reg_sp-48)
	//console.log("SVG SteTransform: "+t0x+", "+t0y+", "+txx+", "+txy+", "+tyx+", "+tyy)
	ctx.setTransform(txx, txy, tyx, tyy, t0x, t0y);
	return 0;
}

function SC_SVGDraw()
{
	let index=GetInt(reg_sp-4)
	SC_DrawVGA(ctx, Scinc_SVGsByIndex[index], null, 0)
}

function SC_SVGDrawAnimated()
{
	let svgindex=GetInt(reg_sp-4)
	let animindex=GetInt(reg_sp-8)
	let time=GetFloat(reg_sp-16)
	SC_DrawVGA(ctx, Scinc_SVGsByIndex[svgindex], Scinc_AnimsByIndex[animindex],time)
	//SC_SVGDraw(ctx,Scinc_SVGsByIndex[svgindex],Scinc_AnimsByIndex[animindex],time)
	//return SC_SVGDraw(ctx,Scinc_SVGsByIndex[svgindex],null,0)
}

function SC_LoadRes(name, path, dest, destByIndex, index, callback)
{
	if(typeof SC_Data !== 'undefined')
	{
		if(typeof SC_Data[path] !== 'undefined')
		{
			console.log("Resource loaded locally '"+name+"'")
			dest[name]=SC_Data[path]
			return
		}
	}
	console.log("Resource requested '"+path+"'")
	promises.push(fetch(path,{mode: 'no-cors'}).then(result=>result.json()).then(data=>{
		dest[name]=data
		destByIndex[index]=data
		console.log("Resource received '"+name+"'='"+path+"'")
		callback(dest[name])
	}))
}

function SC_Timeline(timeline,t,times)
{
	switch(timeline)
	{
		case 'clamp':
		{
			if(t<times[0])
				return times[0]
			else if(t>times[times.length-1])
				return times[times.length-1]
			return t
		}
		break
		case 'extra':
			return t
		break
		case 'repeat':
		{
			let t0=t-times[0]
			let dt=times[times.length-1]-times[0]
			t0=t0-Math.floor(t0/dt)*dt
			return t0+times[0]
		}
		break
		case 'mirror':
		{
			let t0=t-times[0]
			let dt=times[times.length-1]-times[0]
			t0=t0-Math.floor(t0/(dt*2.))*dt*2.
			if(t0/dt>1.0)
				t0=dt*2.0-t0;
			return t0+times[0]
		}
		break
		default:
			return t
	}
}

function SC_GetAnim(anim, t)
{
	let key=0
	t=SC_Timeline(anim.timeline,t,anim.times)

	let times=anim.times
	let keys=anim.keys

	switch(anim.timeline)
	{
		case 'clamp':
			if(t<times[0])
				return keys[0]
			else if(t>times[times.length-1])
				return keys[times.length-1]
			break
		case 'extra':
			if(t<times[0])
				return keys[0]+(keys[1]-keys[0])*(t-times[0])/(times[1]-times[0])
			else if(t>times[times.length-1])
				return keys[times.length-1]+(keys[times.length-2]-keys[times.length-1])
					*(t-times[times.length-1])/(times[times.length-2]-times[times.length-1])
				times[times.length-1]
			break
		case 'repeat':
			{
				let t0=t-times[0]
				let dt=times[times.length-1]-times[0]
				t0=t0-Math.floor(t0/dt)*dt
				t=t0+times[0]
			}
			break
		case 'mirror':
			{
				let t0=t-times[0]
				let dt=times[times.length-1]-times[0]
				t0=t0-Math.floor(t0/(dt*2.))*dt*2.
				if(t0/dt>1.0)
					t0=dt*2.0-t0;
				t=t0+times[0]
			}
			break;
	}

	for(i=0;i<anim.times.length-1;i++)
	{
		if((anim.times[i]<t)&&(anim.times[i+1]>t))
		{
			dt=(t-anim.times[i])/(anim.times[i+1]-anim.times[i])
			switch(anim.interp)
			{
				case 'sin':
					dt=0.5*(1.0+Math.sin((dt-0.5)*Math.PI))
					key=anim.keys[i]*(1-dt)+anim.keys[i+1]*dt
					break
				case 'linear':
					key=anim.keys[i]*(1-dt)+anim.keys[i+1]*dt
					break
				default:
					key=anim.keys[i]*(1-dt)+anim.keys[i+1]*dt
			}
			break;
		}
	}
	return key
}


function SC_AnimTransform(ctx,par,vg,vis,anim,t)
{
	switch(par)
	{
	case 'rotation':
		ctx.transform(...vg.pivot)
		ctx.rotate(SC_GetAnim(anim,t[0]))
		ctx.transform(...vg.invpivot)
		break;
	case 'x':
		ctx.transform(...vg.pivot)
		ctx.translate(SC_GetAnim(anim,t[0]),0)
		ctx.transform(...vg.invpivot)
		break;
	case 'y':
		ctx.transform(...vg.pivot)
		ctx.translate(0,SC_GetAnim(anim,t[0]))
		ctx.transform(...vg.invpivot)
		break;
	case 'scale_x':
		ctx.transform(...vg.pivot)
		ctx.scale(SC_GetAnim(anim,t[0]),1)
		ctx.transform(...vg.invpivot)
		break;
	case 'scale_y':
		ctx.transform(...vg.pivot)
		ctx.scale(1,SC_GetAnim(anim,t[0]))
		ctx.transform(...vg.invpivot)
		break;
	case 'visibility':
		vis[0]=(SC_GetAnim(anim,t[0])>0.5)
		break;
	case 't':
		t[0]=SC_GetAnim(anim,t[0])
		break;
	default:
	}
}

function SC_Animate(ctx,vga,anims,t,vis)
{
	if(!anims)
		return
	if(vga.id in anims.targets)
	{
		for(anim of anims.targets[vga.id].animators)
		{
			SC_AnimTransform(ctx,anim.par,vga,vis,anim,t)
		}
	}
}

function SC_DrawVGA_R(ctx, style, vg, root, order, anims, t)
{
	order[0]++
	if('path' in vg)
	{
		ctx.save()
		if('transform' in vg)
			ctx.transform(...vg.transform)
		if('offset' in vg)
			ctx.transform(...vg.offset)
		let ta=[t]
		let vis=[true]
		SC_Animate(ctx,vg,anims,ta,vis)
		if(vis[0])
		{
			let gstyle = {...style}
			if('style' in vg)
				Object.assign(gstyle,vg.style)
			let skipstroke=false
			if('fill_gradient' in gstyle)
			{
				skipstroke=true
				ctx.fillStyle = root.gradients[gstyle.fill_gradient].ng
				ctx.save()
				ctx.transform(...(root.gradients[gstyle.fill_gradient].transform))
				ctx.fill(vg.gpath)
				ctx.restore()
			}
			else if('fill' in gstyle)
			{
				ctx.fillStyle = gstyle.fill
				ctx.fill(vg.path)
			}
			if('stroke_width' in gstyle)
			{
				ctx.lineWidth = gstyle.stroke_width
			}

			if('stroke_gradient' in gstyle)
			{
				ctx.strokeStyle = root.gradients[gstyle.stroke_gradient].ng
				ctx.stroke(vg.path)
			}else 
			if('stroke' in gstyle)
			{
				ctx.strokeStyle = gstyle.stroke
				ctx.stroke(vg.path)
			}
		}
		ctx.restore()
	}
	else if('g' in vg)
	{
		ctx.save()
		if('transform' in vg)
			ctx.transform(...vg.transform)
		if('offset' in vg)
			ctx.transform(...vg.offset)
		let ta=[t]
		let vis=[true]
		SC_Animate(ctx,vg,anims,ta,vis)
		if(vis[0])
		{
			let gstyle = {...style}
			Object.assign(gstyle,vg.style)
			vg.g.forEach(element => SC_DrawVGA_R(ctx,gstyle,element,root,order,anims,ta[0]))
		}
		ctx.restore()
	}
	else if('use' in vg)
	{
		ctx.save()
		if('transform' in vg)
			ctx.transform(...vg.transform)
		if('offset' in vg)
			ctx.transform(...vg.offset)
		let ta=[t]
		let vis=[true]
		SC_Animate(ctx,vg,anims,ta,vis)
		if(vis[0])
		{
			let gstyle = {...style}
			Object.assign(gstyle,vg.style)
			let link=root.map[vg.use]
			SC_DrawVGA_R(ctx,gstyle,link,root,order,anims,ta[0])
		}
		ctx.restore()
	}
}

function SC_DrawVGA(ctx, vg, anims, t)
{
	if(vg == null)
		return
	ctx.globalAlpha=1
	SC_DrawVGA_R(ctx, {}, vg, vg, [0], anims, t)
}


function SC_InvTransform(m)
{
	let def=1.0/((m[0])*(m[3])-(m[1])*(m[2]));
	return [m[3]*def, -m[1]*def, -m[2]*def, m[0]*def, ((m[5])*(m[2])-(m[4])*(m[3]))*def, ((m[4])*(m[1])-(m[5])*(m[0]))*def]
}

function SC_ListTree(root, node)
{
	if(!('map' in root))
	{
		root.map=new Array;
	}
	root.map[node.id]=node
	node.invpivot = SC_InvTransform(node.pivot)
	if('g' in node)
	{
		node.g.forEach(subnode=>{SC_ListTree(root, subnode)})
	}
	if('path' in node)
	{
		node.path=new Path2D(node.path)
	}
	if('gpath' in node)
	{
		node.gpath=new Path2D(node.gpath)
	}
}

function SC_PrepareJVG(ctx, v)
{
	SC_ListTree(v,v)
	if('gradients' in v)
	{
		v.gradients.forEach(g => {
			var ng
			if(g.type==1)
			{
				ng=ctx.createLinearGradient(0,0,1,0)
			}
			else if(g.type==2)
			{
				ng=ctx.createRadialGradient(0,0,0,0,0,1)
			}
			g.stops.forEach(c =>
			{
					ng.addColorStop(c[0],c[1])
			})
			g.ng=ng
		});
	}
}

function SC_PrepareAnim(anims)
{
	anims.targets={}
	anims.forEach(a => {
		anims.targets[a.target]=a
	});
}

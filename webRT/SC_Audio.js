var scbuf=[]
class ScincAudioProcessor extends AudioWorkletProcessor
{
	constructor()
	{
		super();
		this.port.onmessage = function(e)
		{
			scbuf.push(e.data)
		}

		for(let i=0;i<10;i++)
			this.port.postMessage(0)
		console.log("Created audio processor")
	}
	
	static get parameterDescriptors()
	{
		return [];
	}

	process(inputList, outputList, parameters)
	{
		if(scbuf.length>0)
		for(let o=0;o<1;o++)
		{
			let output = outputList[o];
			let channelCount = output.length
			//for (let channel = 0; channel < 2; channel++)
			{
				let sampleCount = output[0].length;
				let o0=output[0];
				let o1=output[1];
				for (let i = 0; i < sampleCount; i++)
				{
					let l=0
					let r=0
					l=scbuf[0][i*2]
					r=scbuf[0][i*2+1]
					o0[i] = l;
					o1[i] = r;
				}
			}
			scbuf.shift()
		}
		this.port.postMessage(1)
		return true;
	}
}

registerProcessor("ScincAudio", ScincAudioProcessor);
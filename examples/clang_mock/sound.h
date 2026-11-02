// clangd mock of the Scinc intrinsic "sound.h". Never read by Scinc: the
// interpreter handles this include itself. Signatures come from `Scinc -hdr`.
#ifndef SOUND_H
#define SOUND_H

class CSound
{
public:
	int snd_add(int);
	void snd_data(int,int,float,float);
	void snd_out(float,float);
	void snd_out_buf(float*,int);
	static int snd_bufhealth();
	void snd_play(int);
	void Poll();
};

#endif // SOUND_H

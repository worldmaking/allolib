#include "al/core.hpp"
#include "al/core/sound/al_Vbap.hpp"
#include "al/core/sound/al_Speaker.hpp"
#include "al/util/al_Font.hpp"
#include "al/util/al_AlloSphereSpeakerLayout.hpp"

#include <atomic>
#include <vector>

using namespace al;
using namespace std;

#define BLOCK_SIZE (2048)

static SpeakerLayout speakerLayout;
static Vbap *panner;
static float speedMult = 0.01f;

static float srcElev = 1.6f;

static Vec3d srcpos(0.0,0.0,0.0);
static atomic<float> *mPeaks;

////Currently bypasses al_AudioScene

static void audioCB(AudioIOData& io){
	// Render scene
	static unsigned int t = 0;
	double sec;
	float srcBuffer[BLOCK_SIZE];

	while (io()) {
		int i = io.frame();
		float env = (22050 - (t % 22050))/22050.0;
		sec = (t / io.fps());
		// Signal is computed every sample
		srcBuffer[i] = 0.5 * sin(sec*220*M_2PI) * env;;
		++t;
	}
	// But the positions can be computed once per buffer
	float tta = sec*speedMult*M_2PI + M_2PI;
	float x = 12.0*cos(tta);
	float y = 12.0*sin(tta);
	float z = 4.0*sin(2.8 * tta);

	srcpos = Vec3d(x,y,srcElev + z);

	panner->renderBuffer(io, srcpos, srcBuffer, BLOCK_SIZE);

	// Now compute RMS to display the signal level for each speaker
	Speakers &speakers = speakerLayout.speakers();
	for (int i = 0; i < speakers.size(); i++) {
		mPeaks[i].store(0);
	}
	for (int speaker = 0; speaker < speakers.size(); speaker++) {
		float rms = 0;
		for (int i = 0; i < io.framesPerBuffer(); i++) {
			int deviceChannel = speakers[speaker].deviceChannel;
			if(deviceChannel < io.channelsOut()) {
				float sample = io.out(speakers[speaker].deviceChannel, i);
				rms += sample * sample;
			}
		}
		rms = sqrt(rms/io.framesPerBuffer());
		mPeaks[speaker].store(rms);
	}
}


//Converts spatial coordinate system to Allocore's OpenGL coordinate system
Vec3d convertCoords(Vec3d v){
	Vec3d returnVector;
	returnVector.x = v.x;
	returnVector.y = v.z;
	returnVector.z = -v.y;
	return returnVector;
}

class MyApp : public App
{
	Font mFont;
	Mesh mText;

	Mesh mSpeakerMesh;
	vector<Mesh> mVec;

	vector<Vec3d> sCoords;
	vector<int>  sChannels;

	Light mLight;


public:
	MyApp(vector<SpeakerTriple> sts)  {

		//Setup the triangles
		for(int j = 0; j < sts.size();++j){

			SpeakerTriple s = sts[j];
			Mesh* mesh = new Mesh();
			mesh->primitive(Mesh::TRIANGLES);

			for(int i = 0; i < 3; ++i){
				Vec3d coords = convertCoords(s.vec[i]);
				mesh->vertex(coords.x,coords.y,coords.z);
			}

			Color c(rnd::uniform(1.f,0.1f), rnd::uniform(), rnd::uniform(),0.4);
			for(int i = 0;i < 3;++i){
				mesh->color(c);
			}

			mVec.push_back(*mesh);

		}
	}

	void onCreate() {
		mFont.load("allocore/share/fonts/VeraMoIt.ttf", 20);
		mLight.pos(0,6,0);

		addSurface(mText,0,4);

		Speakers sp = speakerLayout.speakers();
		addSphere(mSpeakerMesh);
		for(int i = 0; i <speakerLayout.speakers().size();i++ ){
			Speaker s = sp[i];
			sChannels.push_back(s.deviceChannel);
			sCoords.push_back(convertCoords(s.vec()));
		}

		for(int i = 0; i < mVec.size(); ++i){
			mVec[i].decompress();
			mVec[i].generateNormals();
		}

		nav().pos(0, 2, 45);
	}


	void onDraw(Graphics& g){
		g.clear(0);
		g.blendAdd();

		//Draw the triangles
		g.meshColor();
		for(int i = 0; i < mVec.size(); ++i){
			g.draw(mVec[i]);
		}

		//Draw the source
		g.pushMatrix();
		g.translate(convertCoords(srcpos));
		g.scale(0.1);
		g.color(1);
		g.draw(mSpeakerMesh);
		g.popMatrix();

		//Draw the speakers
		for(int i = 0; i < sCoords.size(); ++i){

			g.pushMatrix();

			g.translate(sCoords[i]);
			float peak = mPeaks[i].load();
			g.scale(0.02 + 0.04 * peak * 30);

			int chan = sChannels[i];
			// mFont.write(mText,to_string(chan));
			// mFont.texture().bind();
			// g.draw(mText);
			// mFont.texture().unbind();
			mFont.render(g, to_string(chan));

			g.color(1);
			g.draw(mSpeakerMesh);
			g.popMatrix();
		}
	}
};



int main (int argc, char * argv[]){

	float rad = 10.f;
	SpeakerLayout tempLayout;
	tempLayout= AlloSphereSpeakerLayout();
	Speakers spks = tempLayout.speakers();

	
	for(Speaker s : spks) {
		s.radius = rad;
		speakerLayout.addSpeaker(s);
	}
	speakerLayout.addSpeaker(Speaker(12, 0, 90, rad)); // Phantom speakers
	speakerLayout.addSpeaker(Speaker(13, 0, -90, rad)); // Phantom speakers

	panner = new Vbap(speakerLayout, true);
	// vector<int> assignedSpeakersTop = {0,1,2,3,4,5,6,7,8,9,10,11};
	// panner->makePhantomChannel(12, assignedSpeakersTop);
	panner->makePhantomChannel(12, {0,1,2,3,4,5,6,7,8,9,10,11}); // assigned speakers top
	// vector<int> assignedSpeakersBottom = {0,1,2,3,4,5,6,7,8,9,10,11};
	// panner->makePhantomChannel(13, assignedSpeakersBottom);
	panner->makePhantomChannel(13, {0,1,2,3,4,5,6,7,8,9,10,11}); // assigned speakers bottom	
	panner->print();

	//Determine number of output channels
	Speakers allSpeakers = speakerLayout.speakers();
	int highestChannel = 0;
	for(Speaker s:allSpeakers){
		if(s.deviceChannel > highestChannel){
			highestChannel = s.deviceChannel;
		}
	}

	mPeaks = new atomic<float>[speakerLayout.speakers().size()];

	int outputChannels = highestChannel + 1;

    AudioDevice::printAll();
    AudioDevice dev(12);
	AudioIO audioIO;
    audioIO.init(audioCB, nullptr, BLOCK_SIZE, 44100, outputChannels, 0);
	audioIO.device(dev);
    audioIO.open();
	audioIO.start();

	MyApp(panner->triplets()).start();
	getchar();

	audioIO.stop();
	delete mPeaks;

	return 0;
}




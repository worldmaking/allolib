/*	Graphics Library of Views (GLV) - GUI Building Toolkit
	See COPYRIGHT file for authors and license information */

#include "al/glv/glv_sliders.h"
#include "al/core/graphics/al_Mesh.hpp"

namespace glv{

Sliders::Sliders(const Rect& r, int nx, int ny, bool dragSelect)
:	Widget(r, 0.5, false, false, false),
	mOri(AUTO), mAcc(0)
{
	data().resize(Data::DOUBLE, nx,ny);
	property(SelectOnDrag, dragSelect);
}


bool Sliders::onEvent(Event::t e, GLV& g){
	if(!Widget::onEvent(e,g)) return false;
	
	switch(e){
		case Event::MouseDrag:
			// if drag settable
			if(enabled(SelectOnDrag)){
				selectSlider(g, false);
			}
			if(g.mouse().right() || g.mouse().left()) {
				// accumulate differences
				mAcc += diam()*(vertOri() ? -g.mouse().dy()/h*sizeY() : g.mouse().dx()/w*sizeX()) * g.mouse().sens();
				setValue(mAcc);
			}
			return false;
			
		case Event::MouseDown:
			selectSlider(g, true);
			return false;
			
		case Event::KeyDown:{
			switch(g.keyboard().key()){
			case 'x':
			case 'a': setValue(getValue() + diam()/32.); return false;
			case 'z': setValue(getValue() - diam()/32.); return false;
			default:;
			}
			break;
		
		case Event::KeyUp:
		case Event::MouseUp: return false;
		}

		default:;
			
	}
	return true;
}

void Sliders::selectSlider(GLV& g, bool click){

	const Mouse& m = g.mouse();
	
	int oldIdx = selected();
	selectFromMousePos(g);
	int idx = selected();
	
	float val = vertOri() ? (1-(m.yRel()/h*sizeY() - selectedY())) : (m.xRel()/w*sizeX() - selectedX());
	val = float(toInterval(val));
	
	// if left-button, set value
	if(m.left() && !m.right()){
		setValue(val);
	}
	
	// if click or new slider, reset accumulator
	if(click || (oldIdx != idx)){
		if(m.left() && !m.right()) mAcc = val;
		else mAcc = getValue(idx);
	}
}


// Slider2D

Slider2D::Slider2D(const Rect& r, double valX, double valY, space_t knobSize)
:	SliderVector<2>(r), mKnobSize(knobSize), mConstrainKnob(true)
{
	setValue(valX, 0);
	setValue(valY, 1);
}

bool Slider2D::onEvent(Event::t e, GLV& g){

	switch(e){
		case Event::MouseDrag:
			valueAdd( g.mouse().dx()/w * diam() * g.mouse().sens(), 0);
			valueAdd(-g.mouse().dy()/h * diam() * g.mouse().sens(), 1);
			return false;
			
		case Event::MouseDown:
			if(g.mouse().left() && !g.mouse().right()){
				setValue(toInterval(    g.mouse().xRel()/w), 0);
				setValue(toInterval(1.f-g.mouse().yRel()/h), 1);
			}
			return false;
			
		case Event::MouseUp:
			clipAccs();
			return false;
		case Event::KeyUp: return false;
		
		case Event::KeyDown:
			switch(g.keyboard().key()){
				case 'x': valueAdd(-diam()/w, 0); return false;
				case 'c': valueAdd( diam()/w, 0); return false;
				case 'a': valueAdd( diam()/h, 1); return false;
				case 'z': valueAdd(-diam()/h, 1); return false;
				default:;
			}
			break;
		default:;
	}
	return true;
}


/*
static void drawQuad(const Slider2D& s){
	using namespace glv::draw;
	float posX = s.w * s.getValue(0);
	float posY = s.h * (1.f - s.getValue(1));
	
	color(s.colors().fore);
	
	begin(Quads);
		vertex(0.f, posY);
		vertex(posX, s.h);
		vertex(s.w, posY);
		vertex(posX, 0.f);
	end();
	
	// clock hand
	//shape(Lines, posX, posY, s.w*0.5, s.h*0.5);
}
*/



SliderRange::SliderRange(const Rect& r, double val1, double val2)
:	SliderVector<2>(r), mDragMode(0), mJump(0.1)
{
	endpoints(val1, val2);
}

double SliderRange::center() const { return (getValue(0) + getValue(1))*0.5; }
double SliderRange::jump() const { return mJump; }
double SliderRange::range() const { return getValue(1)-getValue(0); }

SliderRange& SliderRange::center(double v){ return centerRange(v, range()); }

SliderRange& SliderRange::centerRange(double c, double r){
	double mn = c-(r/2.);
	double mx = mn+r;
	// adjust min/max values to preserve range
	if(mn<min()){ mn=min(); mx = mn+r; }
	if(mx>max()){ mx=max(); mn = mx-r; }

	return endpoints(mn,mx);
}

SliderRange& SliderRange::endpoints(double min, double max){
	glv::sort(min,max);
	setValue(min,0);
	setValue(max,1);
	return *this;
}

SliderRange& SliderRange::jump(double v){ mJump=v; return *this; }
SliderRange& SliderRange::range(double v){ return centerRange(center(), v); }

SliderRange& SliderRange::jumpBy(float v){
	return center(center() + jump()*v*interval().diameter());
}

bool SliderRange::onEvent(Event::t e, GLV& g){

	float value0 = float(to01(getValue(0)));
	float value1 = float(to01(getValue(1)));
	
	//printf("%f %f\n", value0, value1);

//	float dv = (w>h) ? g.mouse().dx()/w : g.mouse().dy()/h;
//	float mp = (w>h) ? g.mouse().xRel()/w : g.mouse().yRel()/h;
	float dv = (w>h) ? g.mouse().dx()/w : -g.mouse().dy()/h; // REV: flip y
	float mp = (w>h) ? g.mouse().xRel()/w : 1.f - g.mouse().yRel()/h; // REV: flip y
	float d1 = mp-value0; if(d1<0) d1=-d1;
	float d2 = mp-value1; if(d2<0) d2=-d2;
	float rg = float(range());
	float endRegion = 4.0f/(w>h ? w:h);

	switch(e){
	case Event::MouseDown:
		
		// NOTE: There is more than one way we might want to move the slider 
		// on a click. We can set its center or increment its center in the
		// direction of the click. Also, we may not want to move the slider
		// if the click lands within the range...
		if(g.mouse().left()){
			float v1 = value0;
			float v2 = value1;
			float center_ = float(to01(center()));
			
			// click outside of range
			if(mp<(v1-endRegion) || mp>(v2+endRegion)){
				float dc = mp - center_;		// signed distance from click to center of slider
//				float dcAbs = dc<0 ? -dc : dc;
//				if(jump() > dcAbs){
//					center(toInterval(mp));
//				}
//				else{
//					center(toInterval(center_ + (dc<0 ? -jump() : jump())));
//				}
				jumpBy(dc<0 ? -1.0f : 1.0f);
				mDragMode=0;
			}
			
			// click on lower edge
			else if(mp > (v1-endRegion) && mp < (v1+endRegion)){
				mDragMode=1;
			}
			
			// click on upper edge
			else if(mp > (v2-endRegion) && mp < (v2+endRegion)){
				mDragMode=2;
			}
			
			// click on range
			else{
				mDragMode=3;
			}
		}
		
		return false;
	
	case Event::MouseDrag:
		dv *= float(diam() * g.mouse().sens());
		if(3==mDragMode){	// clicked on edge of bar
			valueAdd(dv, 0, min(), max()-rg);
			valueAdd(dv, 1, min()+rg, max());
		}
		else if(1==mDragMode) valueAdd(dv, 0);
		else if(2==mDragMode) valueAdd(dv, 1);

		return false;

	case Event::MouseUp:
		if(3==mDragMode){
			mAcc[0] = glv::clip(mAcc[0], max()-rg, min());
			mAcc[1] = glv::clip(mAcc[1], max(), min()+rg);
		}
		else if(1==mDragMode) mAcc[0] = glv::clip(mAcc[0], max(), min());
		else if(2==mDragMode) mAcc[1] = glv::clip(mAcc[1], max(), min());
		return false;

	default:;
	}

	return true;
}



#if 0


FunctionGraph::FunctionGraph(const Rect& r, int nKnots, int res)
: View(r), mTension(0.), mKnobSize(3), mCurrentKnot(-1), mNKnots(nKnots), mKnots(0)
{
	//minimum 3 knots
	if(mNKnots < 3) mNKnots = 3;
	
	mKnots = new Knot[mNKnots];
	for(int i=0; i < mNKnots; i++) {
		mKnots[i].x = ((float)i)/(mNKnots-1);
		mKnots[i].y = mKnots[i].x*mKnots[i].x;
	}
	
	for(int i=0; i < mNKnots-1; i++) {
		mCurves.push_back(new Curve(res));
	}
	calcCurves();
}

FunctionGraph::~FunctionGraph()
{
	if(mKnots) delete[] mKnots;
	std::vector<Curve *>::iterator it = mCurves.begin();
	std::vector<Curve *>::iterator it_e = mCurves.end();
	for(; it != it_e; ++it){
		delete *it;
	}
}

void FunctionGraph::calcCurves()
{
	std::vector<Curve *>::iterator it = mCurves.begin();
	std::vector<Curve *>::iterator it_e = mCurves.end();
	int i=0;
	for(; it != it_e; ++it){
		//for now we duplicate boundry points
		int idx0, idx1, idx2, idx3;
		
		if(i==0) {
			idx0 = idx1 = 0;
			idx2 = 1;
			idx3 = 2;
		}
		else if(i == (mNKnots-2)) {
			idx0 = i-1;
			idx1 = i;
			idx2 = idx3 = mNKnots-1;
		}
		else {
			idx0 = i-1;
			idx1 = i;
			idx2 = i+1;
			idx3 = i+2;
		}
	
		Curve &c = *(*it);
	
		float mu = 0.;
		float dmu = 1./(float)(c.size()-1);
		for(int t = 0; t < c.size(); t++) {
			c[t] = HermiteInterpolate(mKnots[idx0].y, mKnots[idx1].y, mKnots[idx2].y, mKnots[idx3].y, mu, mTension, 0);
			mu += dmu;
		}
		
		i++;
	}
}

void FunctionGraph::eval(int n, float *vals)
{
	float dx = 1.f/(n-1);
	float x = 0;
	int idx = 0;
	int k = 0;
	vals[idx] = mKnots[k].y;
	
	int idx0 = 0;
	int idx1 = 0;
	int idx2 = 1;
	int idx3 = 2;

	for(; idx < n-1; idx++) {
		if(x > mKnots[k+1].x) {
			while(x > mKnots[k+1].x) {
				k++;
			}
			
			if(k == (mNKnots-2)) {
				idx0 = k-1;
				idx1 = k;
				idx2 = idx3 = mNKnots-1;
			}
			else {
				idx0 = k-1;
				idx1 = k;
				idx2 = k+1;
				idx3 = k+2;
			}
		}
		
		float mu = (x - mKnots[k].x) / (mKnots[k+1].x - mKnots[k].x);
		vals[idx] = HermiteInterpolate(mKnots[idx0].y, mKnots[idx1].y, mKnots[idx2].y, mKnots[idx3].y, mu, mTension, 0);
		
		x += dx;
	}
	
	vals[idx] = mKnots[k].y;
}

bool FunctionGraph::onEvent(Event::t e, GLV& glv)
{
	switch(e){
	case Event::MouseDrag: {
		if(glv.mouse().left() && mCurrentKnot >= 0) {
			if(mCurrentKnot == 0 || mCurrentKnot == (mNKnots-1)) {
				mKnots[mCurrentKnot].y = 1.f-(glv.mouse().yRel()/h);
				mKnots[mCurrentKnot].y = (mKnots[mCurrentKnot].y < 0.f) ? 0.f : 
												((mKnots[mCurrentKnot].y > 1.f) ? 1.f : mKnots[mCurrentKnot].y);
			}
			else {
				mKnots[mCurrentKnot].x = (glv.mouse().xRel()/w);
				mKnots[mCurrentKnot].y = 1.f-(glv.mouse().yRel()/h);
				
				mKnots[mCurrentKnot].x = (mKnots[mCurrentKnot].x < 0) ? 0 : 
											((mKnots[mCurrentKnot].x > 1.) ? 1 : mKnots[mCurrentKnot].x);
				
				mKnots[mCurrentKnot].y = (mKnots[mCurrentKnot].y < 0.) ? 0.f : 
												((mKnots[mCurrentKnot].y > 1.f) ? 1.f : mKnots[mCurrentKnot].y);
				
				//check if we went beyond neighboring knots
				if(mKnots[mCurrentKnot].x < mKnots[mCurrentKnot-1].x && mCurrentKnot != mCurrentKnot+1) {
					Knot kt;
					kt.x = mKnots[mCurrentKnot].x;
					kt.y = mKnots[mCurrentKnot].y;
					
					mKnots[mCurrentKnot].x = mKnots[mCurrentKnot-1].x;
					mKnots[mCurrentKnot].y = mKnots[mCurrentKnot-1].y;
					
					mKnots[mCurrentKnot-1].x = kt.x;
					mKnots[mCurrentKnot-1].y = kt.y;
					
					mCurrentKnot--;
				}
				else if(mKnots[mCurrentKnot].x > mKnots[mCurrentKnot+1].x && mCurrentKnot != mNKnots-2) {
					Knot kt;
					kt.x = mKnots[mCurrentKnot].x;
					kt.y = mKnots[mCurrentKnot].y;
					
					mKnots[mCurrentKnot].x = mKnots[mCurrentKnot+1].x;
					mKnots[mCurrentKnot].y = mKnots[mCurrentKnot+1].y;
					
					mKnots[mCurrentKnot+1].x = kt.x;
					mKnots[mCurrentKnot+1].y = kt.y;
					
					mCurrentKnot++;
				}
			}
			
			calcCurves();
		}
	}
	break;
	
	case Event::MouseDown: {
		if(glv.mouse().left()) {
			mCurrentKnot = knotHitTest(glv.mouse().xRel(), glv.mouse().yRel());
		}
	}
	break;
	
	default:
	break;
	}
	
	return false;
}

int FunctionGraph::knotHitTest(space_t x, space_t y)
{
	int idx = -1;
	float min_dsq = mKnobSize*mKnobSize*2.5f;
	
	for(int k = 0; k < mNKnots; k++) {
		float dx = mKnots[k].x*w - x;
		float dy =(1.f-mKnots[k].y)*h - y;
		float dsq = dx*dx+dy*dy;
		if(dsq < min_dsq) {
			min_dsq = dsq;
			idx = k;
		}
	}
	
	return idx;
}

#endif

} // glv::

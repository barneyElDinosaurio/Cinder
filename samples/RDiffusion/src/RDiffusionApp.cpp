#include "cinder/app/App.h"
#include "Resources.h"
#include "cinder/Camera.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#define SIZE 512

using namespace ci;
using namespace ci::app;
using namespace std;

class RDiffusionApp : public App {
  public:
	static void prepareSettings( Settings *settings );
	void setup() override;
	void update() override;
	void draw() override;
	void keyDown( KeyEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;

  private:
	void resetFBOs();

	params::InterfaceGlRef mParams;

	int             mCurrentFBO, mOtherFBO;
	gl::FboRef      mFBOs[2];
	gl::GlslProgRef mRDShader;
	gl::TextureRef  mTexture;

	vec2 mMouse;
	bool mMousePressed;

	float mReactionU;
	float mReactionV;
	float mReactionK;
	float mReactionF;

	static const int FBO_WIDTH = SIZE, FBO_HEIGHT = SIZE;
};

void RDiffusionApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( SIZE, SIZE );
	settings->setFrameRate( 60.0f );
}

void RDiffusionApp::mouseDown( MouseEvent event )
{
	mMousePressed = true;
}

void RDiffusionApp::mouseUp( MouseEvent event )
{
	mMousePressed = false;
}

void RDiffusionApp::mouseMove( MouseEvent event )
{
	mMouse = event.getPos();
}

void RDiffusionApp::mouseDrag( MouseEvent event )
{
	mMouse = event.getPos();
}

void RDiffusionApp::setup()
{
	mReactionU = 0.25f;
	mReactionV = 0.04f;
	mReactionK = 0.047f;
	mReactionF = 0.1f;

	mMousePressed = false;

	// Setup the parameters
	mParams = params::InterfaceGl::create( "Parameters", ivec2( 175, 100 ) );
	mParams->addParam( "Reaction u", &mReactionU, "min=0.0 max=0.4 step=0.01 keyIncr=u keyDecr=U" );
	mParams->addParam( "Reaction v", &mReactionV, "min=0.0 max=0.4 step=0.01 keyIncr=v keyDecr=V" );
	mParams->addParam( "Reaction k", &mReactionK, "min=0.0 max=1.0 step=0.001 keyIncr=k keyDecr=K" );
	mParams->addParam( "Reaction f", &mReactionF, "min=0.0 max=1.0 step=0.001 keyIncr=f keyDecr=F" );

	mCurrentFBO = 0;
	mOtherFBO = 1;
	mFBOs[0] = gl::Fbo::create( FBO_WIDTH, FBO_HEIGHT, gl::Fbo::Format().colorTexture().disableDepth() );
	mFBOs[1] = gl::Fbo::create( FBO_WIDTH, FBO_HEIGHT, gl::Fbo::Format().colorTexture().disableDepth() );

	mRDShader = gl::GlslProg::create( loadResource( RES_PASS_THRU_VERT ), loadResource( RES_GSRD_FRAG ) );
	mTexture = gl::Texture::create( loadImage( loadResource( RES_STARTER_IMAGE ) ),
	    gl::Texture::Format().wrap( GL_REPEAT ).magFilter( GL_LINEAR ).minFilter( GL_LINEAR ) );
	gl::getStockShader( gl::ShaderDef().texture() )->bind();

	resetFBOs();
}

void RDiffusionApp::update()
{
	const int ITERATIONS = 25;
	// normally setMatricesWindow flips the projection vertically so that the upper left corner is 0,0
	// but we don't want to do that when we are rendering the FBOs onto each other, so the last param is false
	gl::setMatricesWindow( mFBOs[0]->getSize() );
	gl::viewport( mFBOs[0]->getSize() );
	for( int i = 0; i < ITERATIONS; i++ ) {
		mCurrentFBO = ( mCurrentFBO + 1 ) % 2;
		mOtherFBO = ( mCurrentFBO + 1 ) % 2;

		gl::ScopedFramebuffer fboBind( mFBOs[mCurrentFBO] );

		{
			gl::ScopedTextureBind tex( mFBOs[mOtherFBO]->getColorTexture(), 0 );
			gl::ScopedTextureBind texSrcBind( mTexture, 1 );
			gl::ScopedGlslProg    shaderBind( mRDShader );

			mRDShader->uniform( "tex", 0 );
			mRDShader->uniform( "texSrc", 1 );
			mRDShader->uniform( "width", (float)FBO_WIDTH );
			mRDShader->uniform( "ru", mReactionU );
			mRDShader->uniform( "rv", mReactionV );
			mRDShader->uniform( "k", mReactionK );
			mRDShader->uniform( "f", mReactionF );
			gl::drawSolidRect( mFBOs[mCurrentFBO]->getBounds() );
		}

		if( mMousePressed ) {
			gl::ScopedGlslProg shaderBind( gl::getStockShader( gl::ShaderDef().color() ) );
			gl::ScopedColor    col( Color::white() );
			RectMapping        windowToFBO( getWindowBounds(), mFBOs[mCurrentFBO]->getBounds() );
			gl::drawSolidCircle( windowToFBO.map( mMouse ), 25.0f, 32 );
		}
	}
}

void RDiffusionApp::draw()
{
	gl::clear();
	gl::setMatricesWindow( getWindowSize() );
	gl::viewport( getWindowSize() );
	{
		gl::ScopedTextureBind bind( mFBOs[mCurrentFBO]->getColorTexture() );
		gl::drawSolidRect( getWindowBounds() );
	}

	mParams->draw();
}

void RDiffusionApp::resetFBOs()
{
	gl::setMatricesWindow( mFBOs[0]->getSize() );
	gl::viewport( mFBOs[0]->getSize() );
	gl::ScopedTextureBind texBind( mTexture );
	for( int i = 0; i < 2; i++ ) {
		gl::ScopedFramebuffer fboBind( mFBOs[i] );
		gl::drawSolidRect( mFBOs[i]->getBounds() );
	}
}

void RDiffusionApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'r' ) {
		resetFBOs();
	}
}

CINDER_APP( RDiffusionApp, RendererGl, RDiffusionApp::prepareSettings )

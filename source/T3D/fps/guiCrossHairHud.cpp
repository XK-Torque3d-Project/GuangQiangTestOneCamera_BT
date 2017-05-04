//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "gui/core/guiControl.h"
#include "gui/controls/guiBitmapCtrl.h"
#include "console/consoleTypes.h"
#include "scene/sceneManager.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/shapeBase.h"
#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"
#include "platformWin32/pcvr/pcvr.h"


//-----------------------------------------------------------------------------
/// Vary basic cross hair hud.
/// Uses the base bitmap control to render a bitmap, and decides whether
/// to draw or not depending on the current control object and it's state.
/// If there is ShapeBase object under the cross hair and it's named,
/// then a small health bar is displayed.
class GuiCrossHairHud : public GuiBitmapCtrl
{
   typedef GuiBitmapCtrl Parent;

   ColorF   mDamageFillColor;
   ColorF   mDamageFrameColor;
   Point2I  mDamageRectSize;
   Point2I  mDamageOffset;
   /////////////////////////////////
   S32 mPcvrIndex;
   bool b_mainCtrl;
   bool mIsPcvrType;
   /////////////////////////////////

protected:
   void drawDamage(Point2I offset, F32 damage, F32 opacity);

public:
   GuiCrossHairHud();
   /////////////////////////////////
   void onRenderPcvr(const RectI &);
   void onRenderNormal( Point2I, const RectI &);
   /////////////////////////////////

   void onRender( Point2I, const RectI &);

   void setMainCtrl( bool );
   Point2I getRenderPosition();
   static void initPersistFields();
   DECLARE_CONOBJECT( GuiCrossHairHud );
   DECLARE_CATEGORY( "Gui Game" );
   DECLARE_DESCRIPTION( "Basic cross hair hud. Reacts to state of control object.\n"
      "Also displays health bar for named objects under the cross hair." );
};

/// Valid object types for which the cross hair will render, this
/// should really all be script controlled.
static const U32 ObjectMask = PlayerObjectType | VehicleObjectType;


//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT( GuiCrossHairHud );

ConsoleDocClass( GuiCrossHairHud,
   "@brief Basic cross hair hud. Reacts to state of control object. Also displays health bar for named objects under the cross hair.\n\n"
   "Uses the base bitmap control to render a bitmap, and decides whether to draw or not depending "
   "on the current control object and it's state. If there is ShapeBase object under the cross hair "
   "and it's named, then a small health bar is displayed.\n\n"
     
   "@tsexample\n"
		"\n new GuiCrossHairHud()"
		"{\n"
		"	damageFillColor = \"1.0 0.0 0.0 1.0\"; // Fills with a solid red color\n"
		"	damageFrameColor = \"1.0 1.0 1.0 1.0\"; // Solid white frame color\n"
		"	damageRect = \"15 5\";\n"
		"	damageOffset = \"0 -10\";\n"
		"};\n"
   "@endtsexample\n"
   
   "@ingroup GuiGame\n"
);

GuiCrossHairHud::GuiCrossHairHud()
{
   mDamageFillColor.set( 0.0f, 1.0f, 0.0f, 1.0f );
   mDamageFrameColor.set( 1.0f, 0.6f, 0.0f, 1.0f );
   mDamageRectSize.set(50, 4);
   mDamageOffset.set(0,32);

   //////////////////////////////////
   mIsPcvrType = true;
   mPcvrIndex = 0;
   b_mainCtrl = false;
   //////////////////////////////////
}

void GuiCrossHairHud::initPersistFields()
{
   addGroup("Damage");		
   addField( "damageFillColor", TypeColorF, Offset( mDamageFillColor, GuiCrossHairHud ), "As the health bar depletes, this color will represent the health loss amount." );
   addField( "damageFrameColor", TypeColorF, Offset( mDamageFrameColor, GuiCrossHairHud ), "Color for the health bar's frame." );
   addField( "damageRect", TypePoint2I, Offset( mDamageRectSize, GuiCrossHairHud ), "Size for the health bar portion of the control." );
   addField( "damageOffset", TypePoint2I, Offset( mDamageOffset, GuiCrossHairHud ), "Offset for drawing the damage portion of the health control." );
   addField("pcvrIndex", TypeS32, Offset(mPcvrIndex, GuiCrossHairHud ) );
   addField("mainCtrl", TypeBool, Offset( b_mainCtrl, GuiCrossHairHud ) );
   endGroup("Damage");
   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------
void GuiCrossHairHud::onRender(Point2I offset, const RectI &updateRect)
{
	if(mIsPcvrType)
		onRenderPcvr(updateRect);
	else
		onRenderNormal(offset, updateRect);
}

void GuiCrossHairHud::onRenderPcvr(const RectI &updateRect)
{
	if(!mTextureObject)
		return;

	RectI rect = updateRect;

	bool isPlayGuiAwake = false;
	GuiControl *p = NULL;
	if ( Sim::findObject( "PlayGui", p ) )
	{
		if ( p->isAwake() )
		{
			isPlayGuiAwake = true;
		}
	}

	if( gPcvr != NULL )
	{
		switch(mPcvrIndex)
		{
		case 0:
			pt_currentPot0.x = rect.point.x = ::gPcvrPointS0.x;
			pt_currentPot0.y = rect.point.y = ::gPcvrPointS0.y;
			if ( isPlayGuiAwake && b_mainCtrl )
			{
				if ( pt_currentPot0.x < 40 )
				{
					Con::executef( "subYaw", "1" );
				}
				if ( pt_currentPot0.x > 1320 )
				{
					Con::executef( "addYaw", "1" );
				}
				if ( pt_currentPot0.y < 40 )
				{
					Con::executef( "subPitch", "1" );
				}
				if ( pt_currentPot0.y > 728 )
				{
					Con::executef( "addPitch", "1" );
				}
			}
			break;
		case 1:
			pt_currentPot1.x = rect.point.x = ::gPcvrPointS1.x;
			pt_currentPot1.y = rect.point.y = ::gPcvrPointS1.y;
			if ( isPlayGuiAwake && b_mainCtrl )
			{
				if ( pt_currentPot1.x < 40 )
				{
					Con::executef( "subYaw", "1" );
				}
				if ( pt_currentPot1.x > 1320 )
				{
					Con::executef( "addYaw", "1" );
				}
				if ( pt_currentPot1.y < 40 )
				{
					Con::executef( "subPitch", "1" );
				}
				if ( pt_currentPot1.y > 728 )
				{
					Con::executef( "addPitch", "1" );
				}
			}
			break;
		case 2:
		case 3:
			rect.point.x = ::gPcvrPointSAy[mPcvrIndex - 2].x;
			rect.point.y = ::gPcvrPointSAy[mPcvrIndex - 2].y;
			break;
		default:
			rect.point.x = 0;
			rect.point.y = 0;
			break;
		}
	}
	else
	{
		rect.point.x = gMouseScreenPt.x;
		rect.point.y = gMouseScreenPt.y;

		if (  gMouseScreenPt.x < 0)
		{
			gMouseScreenPt.x = 0;
		}
		if ( gMouseScreenPt.x > 1360 )
		{
			gMouseScreenPt.x = 1360;
		}
		if ( gMouseScreenPt.y < 0 )
		{
			gMouseScreenPt.y = 0;
		}
		if ( gMouseScreenPt.y > 768 )
		{
			gMouseScreenPt.y = 768;
		}
	}
	U32 width = mTextureObject->getWidth();
	U32 height = mTextureObject->getHeight();

	rect.point.x -= width / 2;
	rect.point.y -= height / 2;

	switch( mPcvrIndex )
	{
	case 0:
		if( ::gPcvrPointS0.x != ( -1 ) && ::gPcvrPointS0.y != ( -1 ) )
		{
			GFX->setClipRect(rect);
			GFX->getDrawUtil()->clearBitmapModulation();
			GFX->getDrawUtil()->drawBitmapStretch(mTextureObject, rect, GFXBitmapFlip_None, GFXTextureFilterLinear);
		}
		break;
	case 1:
		if( ::gPcvrPointS1.x != ( -1 ) && ::gPcvrPointS1.y != ( -1 ) )
		{
			GFX->setClipRect(rect);
			GFX->getDrawUtil()->clearBitmapModulation();
			GFX->getDrawUtil()->drawBitmapStretch(mTextureObject, rect, GFXBitmapFlip_None, GFXTextureFilterLinear);
		}
		break;
	case 2:
	case 3:
		if( ::gPcvrPointSAy[mPcvrIndex - 2].x != ( -1 ) && ::gPcvrPointSAy[mPcvrIndex - 2].y != ( -1 ) )
		{
			GFX->setClipRect(rect);
			GFX->getDrawUtil()->clearBitmapModulation();
			GFX->getDrawUtil()->drawBitmapStretch(mTextureObject, rect, GFXBitmapFlip_None, GFXTextureFilterLinear);
		}
		break;
	}
/*
	GFX->setClipRect(rect);
	GFX->getDrawUtil()->clearBitmapModulation();
	GFX->getDrawUtil()->drawBitmapStretch(mTextureObject, rect, GFXBitmapFlip_None, GFXTextureFilterLinear);*/
}

void GuiCrossHairHud::onRenderNormal(Point2I offset, const RectI &updateRect)
{
   // Must have a connection and player control object
   GameConnection* conn = GameConnection::getConnectionToServer();
   if (!conn)
      return;
   ShapeBase* control = dynamic_cast<ShapeBase*>(conn->getControlObject());
   if (!control || !(control->getTypeMask() & ObjectMask) || !conn->isFirstPerson())
      return;

   // Parent render.
   Parent::onRender(offset,updateRect);

   // Get control camera info
   MatrixF cam;
   Point3F camPos;
   conn->getControlCameraTransform(0,&cam);
   cam.getColumn(3, &camPos);

   // Extend the camera vector to create an endpoint for our ray
   Point3F endPos;
   cam.getColumn(1, &endPos);
   endPos *= gClientSceneGraph->getVisibleDistance();
   endPos += camPos;

   // Collision info. We're going to be running LOS tests and we
   // don't want to collide with the control object.
   static U32 losMask = TerrainObjectType | InteriorObjectType | ShapeBaseObjectType;
   control->disableCollision();

   RayInfo info;
   if (gClientContainer.castRay(camPos, endPos, losMask, &info)) {
      // Hit something... but we'll only display health for named
      // ShapeBase objects.  Could mask against the object type here
      // and do a static cast if it's a ShapeBaseObjectType, but this
      // isn't a performance situation, so I'll just use dynamic_cast.
      if (ShapeBase* obj = dynamic_cast<ShapeBase*>(info.object))
         if (obj->getShapeName()) {
            offset.x = updateRect.point.x + updateRect.extent.x / 2;
            offset.y = updateRect.point.y + updateRect.extent.y / 2;
            drawDamage(offset + mDamageOffset, obj->getDamageValue(), 1);
         }
   }

   // Restore control object collision
   control->enableCollision();
}


//-----------------------------------------------------------------------------
/**
   Display a damage bar ubove the shape.
   This is a support funtion, called by onRender.
*/
void GuiCrossHairHud::drawDamage(Point2I offset, F32 damage, F32 opacity)
{
   mDamageFillColor.alpha = mDamageFrameColor.alpha = opacity;

   // Damage should be 0->1 (0 being no damage,or healthy), but
   // we'll just make sure here as we flip it.
   damage = mClampF(1 - damage, 0, 1);

   // Center the bar
   RectI rect(offset, mDamageRectSize);
   rect.point.x -= mDamageRectSize.x / 2;

   // Draw the border
   GFX->getDrawUtil()->drawRect(rect, mDamageFrameColor);

   // Draw the damage % fill
   rect.point += Point2I(1, 1);
   rect.extent -= Point2I(1, 1);
   rect.extent.x = (S32)(rect.extent.x * damage);
   if (rect.extent.x == 1)
      rect.extent.x = 2;
   if (rect.extent.x > 0)
      GFX->getDrawUtil()->drawRectFill(rect, mDamageFillColor);
}

Point2I GuiCrossHairHud::getRenderPosition()
{
	Point2I pos;
	if(gPcvr)
	{
		switch ( mPcvrIndex )
		{
		case 0:
			pos.x = mFloor( gPcvrPointS0.x );
			pos.y = mFloor( gPcvrPointS0.y );
			break;
		case 1:
			pos.x = mFloor( pt_currentPot1.x );
			pos.y = mFloor( pt_currentPot1.y );
			break;
		case 2:
		case 3:
			pos.x = mFloor( gPcvrPointSAy[mPcvrIndex-2].x );
			pos.y = mFloor( gPcvrPointSAy[mPcvrIndex-2].y );
			break;
		default:
			pos.x = 0;
			pos.y = 0;
			break;
		}
	}
	else
	{
		pos.x = mFloor( gMouseScreenPt.x );
		pos.y = mFloor( gMouseScreenPt.y );
	}
	return pos;
}

ConsoleMethod( GuiCrossHairHud, getRenderPosition, const char*, 2, 2, "" )
{
	char *returnBuffer = Con::getReturnBuffer(64);
	Point2I	pos;
	pos = object->getRenderPosition();
	dSprintf(returnBuffer,64,"%d %d",pos.x,pos.y );
	return returnBuffer;
}

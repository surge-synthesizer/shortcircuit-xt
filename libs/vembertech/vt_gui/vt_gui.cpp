#include "vt_gui.h"
#include "vt_gui_controls.h"
#include <assert.h>

const int sizer_size = 5;

//=======================================================================================================
//
// vg_rect
//
//=======================================================================================================

//-------------------------------------------------------------------------------------------------------

vg_rect::vg_rect()
{
	x = 0;
	x2 = 0;
	y = 0;
	y2 = 0;
}

//-------------------------------------------------------------------------------------------------------

void vg_rect::flip_if_needed()
{
	if(x>x2) swap(x,x2);	
	if(y>y2) swap(y,y2);
}

//-------------------------------------------------------------------------------------------------------

vg_rect::vg_rect(int x, int y, int x2, int y2)
{
	this->x = x;
	this->y = y;
	this->x2 = x2;	
	this->y2 = y2;
}

//-------------------------------------------------------------------------------------------------------

void vg_rect::set(int x, int y, int x2, int y2)
{
	this->x = x;
	this->y = y;
	this->x2 = x2;	
	this->y2 = y2;
}

//-------------------------------------------------------------------------------------------------------

void vg_rect::offset(int ox, int oy)
{
	x += ox;
	x2 += ox;
	y += oy;
	y2 += oy;
}

//-------------------------------------------------------------------------------------------------------

void vg_rect::inset(int ox, int oy)
{
	x += ox;
	x2 -= ox;
	y += oy;
	y2 -= oy;
}

//-------------------------------------------------------------------------------------------------------

void vg_rect::move(int nx, int ny)
{
	x2 -= x;
	y2 -= y;
	x2 += nx;
	y2 += ny;

	x = nx;
	y = ny;
}

//-------------------------------------------------------------------------------------------------------

void vg_rect::move_centerpivot(int x, int y)
{
	move(x - (get_w()>>1) , y - (get_h()>>1));
}

//-------------------------------------------------------------------------------------------------------

void vg_rect::move_constrained(int nx, int ny, vg_rect r)
{
	nx = max(nx,r.x);
	ny = max(ny,r.y);

	nx = min(nx, r.x2 - get_w());
	ny = min(ny, r.y2 - get_h());
	
	x2 -= x;
	y2 -= y;
	x2 += nx;
	y2 += ny;

	x = nx;
	y = ny;

	bound(r);
	//assert(!bound(r));
}

//-------------------------------------------------------------------------------------------------------

bool vg_rect::bound(vg_rect r)
{	
	bool changed = false;
	if(x>x2)
	{
		x2 = x;
		changed = true;
	}
	if(y>y2)
	{
		y2 = y;
		changed = true;
	}

	if(x < r.x)
	{
		x = r.x;
		changed = true;
	}
	if(x2 < r.x)
	{
		x2 = r.x;
		changed = true;
	}
	if(x > r.x2)
	{
		x = r.x2;
		changed = true;
	}
	if(x2 > r.x2)
	{
		x2 = r.x2;
		changed = true;
	}

	if(y < r.y)
	{
		y = r.y;
		changed = true;
	}
	if(y2 < r.y)
	{
		y2 = r.y;
		changed = true;
	}
	if(y > r.y2)
	{
		y = r.y2;
		changed = true;
	}
	if(y2 > r.y2)
	{
		y2 = r.y2;
		changed = true;
	}	

	return changed;	// true if bound changed the rect
}

//-------------------------------------------------------------------------------------------------------

bool vg_rect::point_within(vg_point p)
{
	return (p.x >= x) && (p.x < x2) && (p.y >= y) && (p.y < y2);
}

//-------------------------------------------------------------------------------------------------------

bool vg_rect::point_within(vg_fpoint p)
{
	int px = (float)p.x;
	int py = (float)p.y;
	return (px >= x) && (px < x2) && (py >= y) && (py < y2);
}

//-------------------------------------------------------------------------------------------------------

bool vg_rect::does_intersect(vg_rect r)
{	
	return (((r.x < x2)&&(r.x2 > x)) || ((x < r.x2)&&(x2 > r.x))) && (((r.y < y2)&&(r.y2 > y)) || ((y < r.y2)&&(y2 > r.y)));
}

//-------------------------------------------------------------------------------------------------------

bool vg_rect::does_encase(vg_rect r)
{
	return (r.x >= x)&&(r.x2 <= x2)&&(r.y >= y)&&(r.y2 <= y2);
}

//=======================================================================================================
//
// vg_surface	
//
//=======================================================================================================

vg_surface::vg_surface()
{
	imgdata = 0;	
	font = 0; 
	owner = 0;
	font = 0;
	DIB = false;
	sizeX = 0;
	sizeXA = 0;
	sizeY = 0;
	sizeYA = 0;
}

//-------------------------------------------------------------------------------------------------------

vg_surface::vg_surface(vg_window *ow)
{
	imgdata = 0;	
	font = 0;
	owner = ow;
	font = 0;
	DIB = false;
	sizeX = 0;
	sizeXA = 0;
	sizeY = 0;
	sizeYA = 0;
}

//-------------------------------------------------------------------------------------------------------

vg_surface::~vg_surface()
{
	assert(imgdata == 0);
}

//-------------------------------------------------------------------------------------------------------

bool vg_surface::create(int sizeX, int sizeY, bool DIB)
{		
	assert(imgdata == 0);
	assert(sizeX > 0);
	assert(sizeY > 0);

	hdc = 0;
	this->sizeX = sizeX;
	this->sizeY = sizeY;
	sizeXA = sizeX;
	if(sizeXA & 3) sizeXA = (sizeXA&0xFFFFFFFC) + 4; //make every row alignable (16-bytes)
	sizeYA = sizeY;

	this->DIB = true;

	if(1)
	{			
		bmpinfo.bmiHeader.biWidth = sizeXA;
		bmpinfo.bmiHeader.biHeight = -sizeY;		// kör man minus så blir det top-left som origo
		bmpinfo.bmiHeader.biBitCount = 32;
		bmpinfo.bmiHeader.biPlanes = 1;
		bmpinfo.bmiHeader.biCompression = BI_RGB;
		bmpinfo.bmiHeader.biSizeImage = 0;
		bmpinfo.bmiHeader.biClrUsed = 1;
		bmpinfo.bmiHeader.biClrImportant = 1;	
		bmpinfo.bmiHeader.biXPelsPerMeter = 3000;
		bmpinfo.bmiHeader.biYPelsPerMeter = 3000;
		bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

		HDC hScreen = GetDC (0);	
		hbmp = CreateDIBSection(hScreen,&bmpinfo,DIB_RGB_COLORS,(void**)&imgdata,NULL,NULL);	
		assert(imgdata != 0);
		ReleaseDC (0, hScreen);
	}
	else
	{
		imgdata = new unsigned int[sizeXA*sizeYA];
		assert(imgdata != 0);
	}	

	return true;
}

//-------------------------------------------------------------------------------------------------------

bool vg_surface::create(string filename)
{
	if(!filename.size()) return false;		
	return (read_png(filename)==0);
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::destroy()
{
	if(!imgdata) return;
	if(DIB)
	{
		DeleteObject(hbmp);
		imgdata = 0;
	}
	else
	{
		delete imgdata;
		imgdata = 0;
	}	
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::clear(unsigned int color)
{
	int i,n=sizeXA*sizeY;	
	for(i=0; i<n; i++)
	{
		imgdata[i] = color;
	}
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::set_pixel(int x, int y, unsigned int color)
{
	x = min(max(x,0),sizeX-1);
	y = min(max(y,0),sizeY-1);
	imgdata[y*sizeXA + x] = color;
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::fill_rect(vg_rect r, unsigned int color, bool passthrualpha)
{		
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int alpha = (color>>24)&0xff;
	if((alpha == 0xff)||passthrualpha)
	{
		for(int y=r.y; y<r.y2; y++)	
		{
			for(int x=r.x; x<r.x2; x++)
			{
				imgdata[y*sizeXA + x] = color;
			}
		}
	}
	else
	{
		for(int y=r.y; y<r.y2; y++)	
		{
			for(int x=r.x; x<r.x2; x++)
			{
				alphablend_pixel(color,imgdata[y*sizeXA + x]);
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::fill_ramp(vg_rect r, unsigned int color, int quadrant)
{
	//
	//  _| 0  |_ 1 

	assert(imgdata);
	
	r.bound(vg_rect(0,0,sizeX,sizeY));	// the ramp will not look proper when clipped this way

	bool vertical = r.get_h() > r.get_w();

	if(vertical)
	{
		int h = 0;	
		int dh = (r.get_w()<<16) / max(r.get_h(), 1);
		if(!(quadrant & 1))
		{
			h = (r.get_w()<<16);
			dh = -dh;
		}
		if(quadrant & 2)
		{
			for(int y=r.y; y<r.y2; y++)
			{
				int x1 = r.x2 - (h>>16);
				for(int x=x1; x<r.x2; x++) alphablend_pixel(color,imgdata[y*sizeXA + x]);
				int alpha = ((color>>24)&0xff)*((h>>8)&0xff);
				alphablend_pixel(color&0x00ffffff | ((alpha<<16)&0xff000000),imgdata[y*sizeXA + max(0,x1-1)]);
				h += dh;
			}
		}
		else
		{
			for(int y=r.y; y<r.y2; y++)
			{
				int x2 = r.x + (h>>16);
				for(int x=r.x; x<x2; x++) alphablend_pixel(color,imgdata[y*sizeXA + x]);
				int alpha = ((color>>24)&0xff)*((h>>8)&0xff);
				alphablend_pixel(color&0x00ffffff | ((alpha<<16)&0xff000000),imgdata[y*sizeXA + x2]);
				h += dh;
			}
		}		
		
	}
	else
	{
		int h = 0;	
		int dh = (r.get_h()<<16) / max(r.get_w(), 1);
		if(quadrant & 1)
		{
			h = (r.get_h()<<16);
			dh = -dh;
		}
		if(quadrant & 2)
		{
			for(int x=r.x; x<r.x2; x++)
			{
				int y2 = r.y + (h>>16);
				for(int y=r.y; y<y2; y++) alphablend_pixel(color,imgdata[y*sizeXA + x]);
				int alpha = ((color>>24)&0xff)*((h>>8)&0xff);
				alphablend_pixel(color&0x00ffffff | ((alpha<<16)&0xff000000),imgdata[y2*sizeXA + x]);
				h += dh;
			}
		}
		else
		{
			for(int x=r.x; x<r.x2; x++)
			{
				int y1 = r.y2 - (h>>16);
				for(int y=y1; y<r.y2; y++) alphablend_pixel(color,imgdata[y*sizeXA + x]);
				int alpha = ((color>>24)&0xff)*((h>>8)&0xff);
				alphablend_pixel(color&0x00ffffff | ((alpha<<16)&0xff000000),imgdata[max(0,y1-1)*sizeXA + x]);
				h += dh;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::draw_rect(vg_rect r, unsigned int color, bool passthrualpha)
{	
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int alpha = (color>>24)&0xff;
	if((alpha == 0xff)||passthrualpha)
	{
		for(int y=r.y; y<r.y2; y++)	
		{			
			imgdata[y*sizeXA + r.x] = color;
			imgdata[y*sizeXA + (r.x2-1)] = color;
		}
		for(int x=r.x; x<r.x2; x++)
		{
			imgdata[r.y*sizeXA + x] = color;
			imgdata[(r.y2-1)*sizeXA + x] = color;
		}
	}
	else
	{
		for(int y=r.y; y<r.y2; y++)	
		{
			alphablend_pixel(color,imgdata[y*sizeXA + r.x]);
			alphablend_pixel(color,imgdata[y*sizeXA + (r.x2-1)]);
		}
		for(int x=r.x; x<r.x2; x++)
		{
			alphablend_pixel(color,imgdata[r.y*sizeXA + x]);
			alphablend_pixel(color,imgdata[(r.y2-1)*sizeXA + x]);
		}
	}
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::set_HDC(HDC nhdc)
{
	hdc = nhdc;
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::blit(vg_rect r, vg_surface *source)
{		
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = min(source->sizeX,min(r.get_w(), sizeX - r.x));
	int h = min(source->sizeY,min(r.get_h(), sizeY - r.y));

	int cxa = source->sizeXA;
		
	// optimize with vector code and/or gpu
	for(int y = 0; y<h; y++)	
	{
		for(int x = 0; x<w; x++)
		{
			imgdata[(x+r.x) + (y+r.y)*sizeXA] = source->imgdata[x + y*cxa];
		}
	}	
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::blitGDI(vg_rect r, vg_surface *source, HDC hdc)
{		
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = min(source->sizeX,min(r.get_w(), sizeX - r.x));
	int h = min(source->sizeY,min(r.get_h(), sizeY - r.y));
	int cxa = source->sizeXA;

	if(!hdc)
	{
		blit(r,source);
		return;
	}
			
	HDC hdcS = CreateCompatibleDC (hdc);
	HGDIOBJ hOldObj = SelectObject (hdcS, source->hbmp);			
	BitBlt (hdc, r.x, r.y, w, h, hdcS, 0, 0, SRCCOPY);
	SelectObject (hdcS, hOldObj);
	DeleteDC (hdcS);
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::blit(vg_rect r, vg_bitmap source)
{
	if(!source.surf) return;
	if(!source.surf->imgdata) return;
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = max(0,min(min(r.get_w(), sizeX - r.x),source.r.get_w()));
	int h = max(0,min(min(r.get_h(), sizeY - r.y),source.r.get_h()));

	int cxa = source.surf->sizeXA;
	unsigned int *srcimage = &source.surf->imgdata[cxa*source.r.y + source.r.x];
	// optimize with vector code and/or gpu
	for(int y = 0; y<h; y++)	
	{
		for(int x = 0; x<w; x++)
		{
			imgdata[(x+r.x) + (y+r.y)*sizeXA] = srcimage[x + y*cxa];
		}
	}
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::blitGDI(vg_rect r, vg_bitmap source, HDC hdc)
{			
	if(!source.surf) return;
	if(!source.surf->imgdata) return;
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = min(min(r.get_w(), sizeX - r.x),source.r.get_w());
	int h = min(min(r.get_h(), sizeY - r.y),source.r.get_h());

	int cxa = source.surf->sizeXA;
			
	HDC hdcS = CreateCompatibleDC (hdc);
	HGDIOBJ hOldObj = SelectObject (hdcS, source.surf->hbmp);			
	BitBlt (hdc, r.x, r.y, w, h, hdcS, source.r.x, source.r.y, SRCCOPY);
	////bool b = AlphaBlend(hdc, r.x, r.y, w, h, hdcS, source.r.x, source.r.y, w, h, bf);
	SelectObject (hdcS, hOldObj);
	DeleteDC (hdcS);	
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::blit_alphablend(vg_rect r, vg_bitmap source)
{
	if(!source.surf) return;
	if(!source.surf->imgdata) return;
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = min(min(r.get_w(), sizeX - r.x),source.r.get_w());
	int h = min(min(r.get_h(), sizeY - r.y),source.r.get_h());

	int cxa = source.surf->sizeXA;
	unsigned int *srcimage = &source.surf->imgdata[cxa*source.r.y + source.r.x];
	// optimize with vector code and/or gpu
	for(int y = 0; y<h; y++)	
	{
		for(int x = 0; x<w; x++)
		{
			alphablend_pixel(srcimage[x + y*cxa],imgdata[(x+r.x) + (y+r.y)*sizeXA]);
		}
	}
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::blit_alphablend(vg_rect r, vg_surface *source)
{		
	if(!source) return;
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = min(r.get_w(), sizeX - r.x);
	int h = min(r.get_h(), sizeY - r.y);
	int cxa = source->sizeXA;
		
	for(int y = 0; y<h; y++)	
	{
		for(int x = 0; x<w; x++)
		{
			alphablend_pixel(source->imgdata[x + y*cxa],imgdata[(x+r.x) + (y+r.y)*sizeXA]);
		}
	}	
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::blit_alphablendGDI(vg_rect r, vg_surface *source, HDC hdc)
{		
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = min(source->sizeX,min(r.get_w(), sizeX - r.x));
	int h = min(source->sizeY,min(r.get_h(), sizeY - r.y));
	int cxa = source->sizeXA;
			
	HDC hdcS = CreateCompatibleDC (hdc);
	HGDIOBJ hOldObj = SelectObject (hdcS, source->hbmp);			
	BLENDFUNCTION bf;
	bf.AlphaFormat = 0; //AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 200;
	bool b = AlphaBlend(hdc, r.x, r.y, w, h, hdcS, 0, 0, w, h, bf);
	SelectObject (hdcS, hOldObj);
	DeleteDC (hdcS);
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::blit_alphablendGDI(vg_rect r, vg_bitmap source, HDC hdc)
{
	if(!source.surf) return;
	if(!source.surf->imgdata) return;
	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = min(min(r.get_w(), sizeX - r.x),source.r.get_w());
	int h = min(min(r.get_h(), sizeY - r.y),source.r.get_h());

	int cxa = source.surf->sizeXA;
			
	HDC hdcS = CreateCompatibleDC (hdc);
	HGDIOBJ hOldObj = SelectObject (hdcS, source.surf->hbmp);			
	BLENDFUNCTION bf;
	bf.AlphaFormat = 0; //AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 200;
	bool b = AlphaBlend(hdc, r.x, r.y, w, h, hdcS, source.r.x, source.r.y, w, h, bf);
	SelectObject (hdcS, hOldObj);
	DeleteDC (hdcS);
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::borderblit(vg_rect r, vg_bitmap source)
{
	if(!source.surf) return;
	if(!source.surf->imgdata) return;

	r.bound(vg_rect(0,0,sizeX,sizeY));

	int cw = min(source.r.get_w()>>1, r.get_w()>>1);
	int ch = min(source.r.get_h()>>1, r.get_h()>>1);
	
	// draw corners
	for(int i=0; i<4; i++)
	{
		vg_bitmap tmp = source;
		vg_rect r2 = r;
		if(i&1)	// right half
		{			
			r2.x = r2.x2 - cw;
			tmp.r.x = tmp.r.x2 - cw;
		}
		else	// left half
		{
			r2.x2 = r2.x + cw;
			tmp.r.x2 = tmp.r.x + cw;
		}
		if(i&2)	// lower half
		{
			tmp.r.y = tmp.r.y2 - ch;	
			r2.y = r2.y2 - ch;
		}
		else	// upper half
		{
			tmp.r.y2 = tmp.r.y + ch;	
			r2.y2 = r2.y + ch;
		}
		
		blit_alphablend(r2,tmp);	
	}	

	// draw edges
	{
		int we = r.get_w() - cw;
		int he = r.get_h() - ch;
		int h = r.get_h();
		int w = r.get_w();
		int y2 = 0;
		for(int y=0; y<h; y++)
		{
			if(y==ch) 
			{
				y = max(ch+1,h-ch);
				y2++;
			}

			unsigned int c = source.surf->imgdata[(cw+source.r.x) + (y2+source.r.y)*source.surf->sizeXA];
			for(int x=cw; x<we; x++)
			{	
				alphablend_pixel(c,imgdata[(x+r.x) + (y+r.y)*sizeXA]);
			}
			y2++;
		}
		int x2 = 0;
		for(int x=0; x<w; x++)
		{
			if(x==cw) 
			{
				x = max(cw+1,w-cw);
				x2++;
			}

			unsigned int c = source.surf->imgdata[(x2+source.r.x) + (ch+source.r.y)*source.surf->sizeXA];
			for(int y=ch; y<he; y++)
			{	
				alphablend_pixel(c,imgdata[(x+r.x) + (y+r.y)*sizeXA]);
			}
			x2++;
		}
	}

	// fill middle
	{
		unsigned int midcolor = source.surf->imgdata[(cw+source.r.x) + (ch+source.r.y)*source.surf->sizeXA];
		vg_rect r2 = r;
		r2.x += cw;
		r2.x2 -= cw;
		r2.y += ch;
		r2.y2 -= ch;
		fill_rect(r2,midcolor);
	}
}

//-------------------------------------------------------------------------------------------------------

inline void fontblend(unsigned int col, unsigned int alpha, unsigned int &dst)
{
	// assume ARGB
	// will be reversed on PPC
	
	alpha += (alpha>>7);	// 256 steps instead of 255
	int alpham1 = 256 - alpha;
	int r = ((col>>16)&0xff)*alpha + ((dst>>16)&0xff)*alpham1;
	int g = ((col>>8)&0xff)*alpha + ((dst>>8)&0xff)*alpham1;
	int b = ((col)&0xff)*alpha + ((dst)&0xff)*alpham1;
	dst = (col&0xff000000) | ((r<<8)&0xff0000) | (g&0xff00) | ((b>>8)&0xff);
	// can be made transparent by setting col alpha
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::draw_text_multiline(vg_rect r, string text, unsigned int color, int halign, int valign)
{
	if(!font) return;
		
	int y=r.y;
	int fontheight = font->cell_height;	
	if(!text.empty())
	{		
		basic_string <char>::size_type a=0,b=0;
		while(b<text.size())
		{
			string entry;
			b = text.find_first_of("\n",a);			
			
			if(b == text.npos) b=text.size();
			entry = text.substr(a,b-a);

			draw_text(vg_rect(r.x,y,r.x2,min(y+fontheight,r.y2)),entry,color,halign,-1);	
			y+=fontheight;
			a=b+1;
		}
	}
}

//-------------------------------------------------------------------------------------------------------

void vg_surface::draw_text(
	vg_rect r, 
	string text, 
	unsigned int color, 
	int halign, 
	int valign,
	int ibeampos, 
	bool IgnoreParsing)
{	
	assert(font);	
	if(!font) return;

	r.bound(vg_rect(0,0,sizeX,sizeY));
	int w = font->cell_width;
	int h = min(font->cell_height,r.get_h());
	int fontXA = font->cell_bytes;

	// Special colour case parsing
	if (!IgnoreParsing)
	{		
		if (text[0] == '}')		// Off value
		{
			color = owner->get_syscolor(col_standard_text_disabled);
			text.erase(0,1);
		}
	}
	
	const unsigned char *cs = (const unsigned char *)text.c_str();
	int x = r.x;
	int ypos = r.y;

	// calculate string width
	int stringwidth = max(0,(text.size()-1))*font->char_spacing;
	int tabs = 0;
	for(int i=0; i<text.size(); i++) 
	{
		if(!cs[i]) break;
		stringwidth += font->char_width[cs[i]];	
		if(cs[i] == '\t') tabs++;
	}

	// calculate maximum width of tabs (if available)
	int tabwidth = 1;
	if(tabs) 
	{
		tabwidth = max(0, r.get_w() - stringwidth) / tabs;
		stringwidth += tabwidth*tabs;
	}

	switch(halign)
	{
	case vg_align_center:
		x += (r.get_w() + 1 - stringwidth)>>1;
		break;
	case vg_align_right:
		x += r.get_w() - stringwidth;
		break;
	}	
	x = max(0,max(r.x,x));

	switch(valign)
	{
	case vg_align_center:
		ypos = r.y + ((r.get_h() - h + 1)>>1);
		break;
	case vg_align_bottom:
		ypos = r.y + r.get_h() - h;
		break;
	}	
	ypos = max(0,ypos);

	unsigned int *dst = &imgdata[ypos*sizeXA];

	if(!ibeampos)
	{
		for(int y=0; y<h; y++) fontblend(color,255, dst[max(0,x-1) + y*sizeXA]);
	}

	for(int i=0; i<text.size(); i++)
	{
		unsigned int c = cs[i];
		if(!c) break;
		unsigned char *src = &font->data[c * font->cell_bytes];
		int cw = font->char_width[c];		
		if(c == '\t') cw += tabwidth;

		for(int xx=0; xx<cw; xx++)
		{						
			if(x>=r.x2) break;
			for(int y=0; y<h; y++)
			{
				fontblend(color,src[xx + y*w], dst[x + y*sizeXA]);
			}
			x++;
		}		
		
		if(((i+1) == ibeampos)&&(ibeampos>=0))
		{
			for(int y=0; y<h; y++) fontblend(color,255, dst[x + y*sizeXA]);
		}

		x += font->char_spacing;
	}
}

//=======================================================================================================
//
// vg_font
//
//=======================================================================================================

vg_font::vg_font()
{
	data = 0;
}

//-------------------------------------------------------------------------------------------------------

vg_font::~vg_font()
{
	delete data;	
}

//-------------------------------------------------------------------------------------------------------

bool vg_font::load(string filename)
{	
	vg_surface surf;	
	if (!surf.create(filename)) return false;	
	cell_width = surf.sizeX >> 4;
	cell_height = (surf.sizeY >> 4)-1;
	int surfheight = cell_height+1;
	cell_bytes = cell_width*cell_height;

	if (data) delete data;
	data = new unsigned char[256 * cell_bytes];

	for(int i=0; i<256; i++) 
	{		
		char_width[i] = cell_width;
		for(int x=0; x<cell_width; x++)
		{
			unsigned int a = (surf.imgdata[x + (i&0xf)*cell_width + (i>>4)*surfheight*surf.sizeXA]) & 0xff;
			if(!a) 
			{
				char_width[i] = x;
				break;
			}
		}		

		for(int y=0; y<cell_height; y++)
		{
			for(int x=0; x<cell_width; x++)
			{
				unsigned int a = (surf.imgdata[x + (i&0xf)*cell_width + (y + 1 + (i>>4)*surfheight)*surf.sizeXA]) & 0xff;
				data[x + y*cell_width + i*cell_bytes] = a;
			}
		}
	}
	char_spacing = 1;
	
	surf.destroy();

	return true;
}

//=======================================================================================================
//
// vg_control
//
//=======================================================================================================


vg_control::vg_control(vg_window *o, int id, TiXmlElement *e)
: surf(o)
{
	owner = o;
	control_id = id;
	parameter_id = 0;
	parameter_subid = 0;
	filter = 0;
	filter_entry = 0;
	debug_drawcount = 0;
	offset = 0;
	offset_amount = 0;
	hidden = false;
	draw_needed = true;
	surf.font = owner->get_font(0);
}

//-------------------------------------------------------------------------------------------------------

vg_control::~vg_control()
{
	surf.destroy();
}

//-------------------------------------------------------------------------------------------------------

void vg_control::draw()
{	
	surf.clear(0x80ff0000);
}

//-------------------------------------------------------------------------------------------------------

string vg_control::decode_vars(string s)
{
	string::size_type pos = s.find("%i",0);
	if(pos != string::npos) 
	{
		char meh[16];
		int sid = parameter_subid + 1;
		if(offset) sid += offset_amount * owner->filters[offset];
		sprintf(meh,"%i",sid);
		s.replace(pos,2,meh);
	}
	return s;
}

//-------------------------------------------------------------------------------------------------------

void vg_control::draw_background()
{
	int subpage = owner->filters[1]&7;

	if(!visible())
	{
		//set_dirty();
		surf.clear(owner->bgcolor);
		return;
	}

	if(owner->altbg[subpage].is_initiated())
	{
		vg_bitmap b;
		b.r = location;
		b.r.bound(vg_rect(0,0,owner->altbg[subpage].sizeX,owner->altbg[subpage].sizeY));
		b.surf = &owner->altbg[subpage];
		surf.blit(vg_rect(0,0,location.get_w(),location.get_h()),b);
	}
	else 
	if(owner->background.is_initiated())
	{
		vg_bitmap b;
		b.r = location;
		b.r.bound(vg_rect(0,0,owner->background.sizeX,owner->background.sizeY));
		b.surf = &owner->background;
		surf.blit(vg_rect(0,0,location.get_w(),location.get_h()),b);
	}
	else surf.clear(owner->bgcolor);	
}

//-------------------------------------------------------------------------------------------------------

void vg_control::set_rect(vg_rect r)
{	
	r.bound(owner->get_rect());
	bool resize = (location.get_h() != r.get_h()) || (location.get_w() != r.get_w());
	location = r;
	if(resize)
	{
		surf.destroy();
		surf.create(location.get_w(),location.get_h());	
		on_resize();
	}	
	draw_needed = true;
}

//-------------------------------------------------------------------------------------------------------

void vg_control::set_dirty(bool redraw_all)
{
	draw_needed = true; 
	// TODO den har crashat här. beror troligtvis på thread-unsafe
	// en csec borde fixa det
	owner->set_child_dirty(control_id,redraw_all);
}

//-------------------------------------------------------------------------------------------------------

void vg_control::serialize_data(TiXmlElement *e)
{
	for(int i=0; i<get_n_parameters(); i++)
	{
		e->SetAttribute(get_parameter_name(i),get_parameter_text(i));	
	}	
}

//-------------------------------------------------------------------------------------------------------

void vg_control::restore_data(TiXmlElement *e)
{
	if(!e) return;
	for(int i=0; i<get_n_parameters(); i++)
	{
		const char *c = e->Attribute(get_parameter_name(i));
		if (c) set_parameter_text(i,c,true);	
	}		
}

//-------------------------------------------------------------------------------------------------------

void vg_control::processevent(vg_controlevent &e)
{	
	switch(e.eventtype)
	{
	case vget_mousedown:
	case vget_mousemove:
		{
			if(e.buttonmask & 1)
			{
				int x = (int) e.x;
				int y = (int) e.y;
				surf.set_pixel(x,y,0xffffffff);	
				set_dirty();			
			}
		}
		break;
	}
}

//-------------------------------------------------------------------------------------------------------

bool vg_control::visible()
{	
	if((filter>0) && (filter < max_vgwindow_filters) && !((1 << owner->filters[filter]) & filter_entry)) return false;
	return true;
}

//-------------------------------------------------------------------------------------------------------

// only uses 16 combinations atm (hex) to make it all easy 
unsigned int bitmask_from_hexlist(const char* str)
{
	unsigned int mask = 0;
	while(*str)
	{
		int c = toupper(*str);
		if(c > 'A') c += 10 - 'A';
		else c -= '0';

		mask |= 1 << c;
		str++;
	}
	return mask;
}

//-------------------------------------------------------------------------------------------------------

void hexlist_from_bitmask(char* str, unsigned int bitmask)
{	
	for(int i=0; i<16; i++)
	{
		if(bitmask & (1<<i))
		{
			if(i>9) *str = 'A' + (i-10);
			else *str = '0' + i;
			str++;
		}
	}
	*str = 0;
}

//-------------------------------------------------------------------------------------------------------
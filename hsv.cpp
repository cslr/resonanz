

namespace whiteice
{
  namespace resonanz
  {
    // values are within range 0-255
    
    void rgb2hsv(const unsigned int r, const unsigned int g, const unsigned int b,
		 unsigned int& h, unsigned int& s, unsigned int& v)
    {
      unsigned char rgbMin, rgbMax;

      rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
      rgbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

      v = rgbMax;
      if (v == 0)
      {
        h = 0;
        s = 0;
	return;
      }

      s = 255 * long(rgbMax - rgbMin) / v;
      if (s == 0)
      {
        h = 0;
        return;
      }

      if (rgbMax == r)
        h = 0 + 43 * (g - b) / (rgbMax - rgbMin);
      else if (rgbMax == g)
        h = 85 + 43 * (b - r) / (rgbMax - rgbMin);
      else
        h = 171 + 43 * (r - g) / (rgbMax - rgbMin);
      
      return;
    }
    

    void hsv2rgb(const unsigned int h, const unsigned int s, const unsigned int v,
		 unsigned int& r, unsigned int& g, unsigned int& b)
    {
      unsigned char region, remainder, p, q, t;

      if (s == 0)
      {
        r = v;
        g = v;
        b = v;
        return;
      }
      
      region = h / 43;
      remainder = (h - (region * 43)) * 6; 

      p = (v * (255 - s)) >> 8;
      q = (v * (255 - ((s * remainder) >> 8))) >> 8;
      t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
      
      switch (region)
      {
      case 0:
	r = v; g = t; b = p;
	break;
      case 1:
	r = q; g = v; b = p;
	break;
      case 2:
	r = p; g = v; b = t;
	break;
      case 3:
	r = p; g = q; b = v;
	break;
      case 4:
	r = t; g = p; b = v;
	break;
      default:
	r = v; g = p; b = q;
	break;
      }
      
      return;
    }
    
    
  }
}

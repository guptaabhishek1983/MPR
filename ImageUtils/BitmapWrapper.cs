using System;
using System.Collections.Generic;

using System.Text;
using System.Runtime.InteropServices;
using System.Drawing.Imaging;
using System.Drawing;
using System.Diagnostics;
using System.Drawing.Drawing2D;

namespace ImageUtils
{
    /// <summary>
    /// 
    /// </summary>
    public unsafe struct PixelData
    {
        public byte blue;
        public byte green;
        public byte red;

        public static void Copy(PixelData* src, PixelData* dest)
        {
            dest->blue = src->blue;
            dest->green = src->green;
            dest->red = src->red;
        }

        public void Invert()
        {
            blue = (byte)(255 - blue);
            green = (byte)(255 - green);
            red = (byte)(255 - red);
        }
    }

    /// <summary>
    /// Summary description for BitmapWrapper.
    /// </summary>
    public unsafe class BitmapWrapper : IDisposable
    {

        #region PInvoke
        [DllImport("kernel32.dll", EntryPoint = "RtlMoveMemory")]
        public static extern void CopyMemory(IntPtr Destination, IntPtr Source,
            [MarshalAs(UnmanagedType.U4)] int Length);
        #endregion

        private Bitmap bitmap;

        // three elements used for MakeGreyUnsafe
        bool isRGBImage = false;
        bool isPadded = false;
        int width;
        BitmapData bitmapData = null;
        Byte* pBase = null;
        private const int bytesPerPixel = 4;

        #region Constructor and destructor
        public BitmapWrapper()
        {
        }

        public BitmapWrapper(Bitmap bitmap)
        {
            this.bitmap = bitmap;
            if (bitmap.PixelFormat == PixelFormat.Format8bppIndexed)
            {
                isRGBImage = false;
                SetGrayscalePalette();
            }
            else
            {
                isRGBImage = true;
            }

        }

        public void Dispose()
        {
            if (null != bitmap)
            {
                bitmap.Dispose();
            }
        }

        ~BitmapWrapper()
        {
        }

        /// <summary>
        /// Constructor for 8bpp grayscale data
        /// </summary>
        /// <param name="pData"></param>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="phName"></param>
        public BitmapWrapper(void* pData, int width, int height, string phName)
        {
            if (pData == null) this.bitmap = null;
            else
            {
                //for coloured images 
                /* Here the order of bytes have been reversed because of the little Endian Big Endian Issues
                 * For Red: expected value is 0x00FF0000 but it comes as 0x0000FF00
                 * For Green: expected value is 0x0000FF00 but it comes as 0x00FF0000
                 * For Blue: expected value is 0x000000FF but it comes as 0xFF000000
                 */
                if (phName.Equals("RGB") || phName.Equals("PALETTE COLOR"))
                {
                    byte* iter = (byte*)pData;
                    isRGBImage = true;
                    this.bitmap = new Bitmap(width, height);
                    LockBitmap();
                    for (int y = 0; y < height; y++)
                    {
                        PixelData* pPixel = PixelAt(0, y);
                        for (int x = 0; x < width; x++)
                        {
                            pPixel->blue = *iter;
                            iter++;
                            pPixel->green = *iter;
                            iter++;
                            pPixel->red = *iter;
                            iter++;
                            iter++;
                            pPixel++;
                        }
                    }
                    UnlockBitmap();
                }
                //for grey images 
                else
                {
                    isRGBImage = false;
                    this.bitmap = new Bitmap(width, height, PixelFormat.Format8bppIndexed);
                    GraphicsUnit unit = GraphicsUnit.Pixel;
                    RectangleF boundsF = bitmap.GetBounds(ref unit);
                    Rectangle bounds = new Rectangle((int)boundsF.X,
                        (int)boundsF.Y,
                        (int)boundsF.Width,
                        (int)boundsF.Height);
                    SetGrayscalePalette();
                    LockBitmap();
                    if (isPadded)
                    {
                        for (int h = 0; h < height; h++)
                        {
                            byte* destData = PixelAtForGrey(0, h);
                            byte* srcData = (byte*)pData + h * width;
                            CopyMemory(new IntPtr(destData), new IntPtr(srcData), width);
                        }
                    }
                    else
                    {
                        CopyMemory(new IntPtr(pBase), new IntPtr(pData), width * height);
                    }
                    UnlockBitmap();
                }
            }
        }
        #endregion

        #region Public properties
        public Bitmap StoredBitmap
        {
            get
            {
                return (bitmap);
            }
            set
            {
                this.bitmap = value;
            }
        }

        public Point PixelSize
        {
            get
            {
                GraphicsUnit unit = GraphicsUnit.Pixel;
                RectangleF bounds = bitmap.GetBounds(ref unit);
                return new Point((int)bounds.Width, (int)bounds.Height);
            }
        }

        /// <summary>
        /// Must use 
        /// </summary>
        public BitmapData BitmapData
        {
            get
            {
                return bitmapData;
            }
        }

        public bool IsRGBImage
        {
            get { return this.isRGBImage; }
            set { this.isRGBImage = value; }
        }
        #endregion

        #region Public methods
        public void LockBitmap()
        {
            GraphicsUnit unit = GraphicsUnit.Pixel;
            RectangleF boundsF = bitmap.GetBounds(ref unit);
            Rectangle bounds = new Rectangle((int)boundsF.X,
                (int)boundsF.Y,
                (int)boundsF.Width,
                (int)boundsF.Height);

            // Figure out the number of bytes in a row
            // This is rounded up to be a multiple of 4
            // bytes, since a scan line in an image must always be a multiple of 4 bytes
            // in length. 
            if (isRGBImage)
            {
                width = (int)boundsF.Width * sizeof(PixelData);
                if (width % 4 != 0)
                {
                    isPadded = true;
                    width = 4 * (width / 4 + 1);
                }
                bitmapData =
                    bitmap.LockBits(bounds, ImageLockMode.ReadWrite, PixelFormat.Format24bppRgb);
                pBase = (Byte*)bitmapData.Scan0.ToPointer();
            }
            else
            {
                width = (int)boundsF.Width;
                if (width % 4 != 0)
                {
                    isPadded = true;
                    width = 4 * (width / 4 + 1);
                }
                bitmapData =
                    bitmap.LockBits(bounds, ImageLockMode.ReadWrite, PixelFormat.Format8bppIndexed);
                pBase = (Byte*)bitmapData.Scan0.ToPointer();
            }
        }

        public PixelData* PixelAt(int x, int y)
        {
            Debug.Assert(isRGBImage, "This Wrapper contains Grey Image. Call PixelAtForGrey instead of this method to get the pixelvalue");
            return (PixelData*)(pBase + y * width + x * sizeof(PixelData));
        }

        public byte* PixelAtForGrey(int x, int y)
        {
            Debug.Assert((!isRGBImage), "This Wrapper contains RGB Image. Call PixelAt instead of this method to get the pixelvalue");
            return (byte*)(pBase + y * width + x);
        }

        public void UnlockBitmap()
        {
            bitmap.UnlockBits(bitmapData);
            bitmapData = null;
            pBase = null;
        }

        /// <summary>
        /// code commented out need to fix it for GreyScale Image before using it 
        /// </summary>
        /// <param name="pData"></param>
        /// <param name="width"></param>
        /// <param name="height"></param>
        public void UpdateImage(void* pData, int width, int height)
        {
            if (this.bitmap == null) return;

            //checking the dimensions with the existing bitmap
            if ((this.bitmap.Width != width) || (this.bitmap.Height != height))
            {
                //need to generate bitmap of new dimensions
                this.isPadded = false;
                this.bitmap.Dispose();

                //creating a new bitmap
                if (this.isRGBImage)
                {
                    this.bitmap = new Bitmap(width, height);
                }
                else
                {
                    this.bitmap = new Bitmap(width, height, PixelFormat.Format8bppIndexed);
                    SetGrayscalePalette();
                }
            }

            //setting the pixel data
            LockBitmap();
            if (this.isRGBImage)
            {
                byte* iter = (byte*)pData;
                for (int y = 0; y < height; y++)
                {
                    PixelData* pPixel = PixelAt(0, y);
                    for (int x = 0; x < width; x++)
                    {
                        pPixel->blue = *iter;
                        iter++;
                        pPixel->green = *iter;
                        iter++;
                        pPixel->red = *iter;
                        iter++;
                        iter++;
                        pPixel++;
                    }
                }
            }
            else
            {
                if (isPadded)
                {
                    for (int h = 0; h < height; h++)
                    {
                        byte* destData = PixelAtForGrey(0, h);
                        byte* srcData = (byte*)pData + h * width;
                        CopyMemory(new IntPtr(destData), new IntPtr(srcData), width);
                    }
                }
                else
                {
                    CopyMemory(new IntPtr(pBase), new IntPtr(pData), width * height);
                }
            }
            UnlockBitmap();

            #region Commented Out
            //			if ((bitmap.Width != width)||(bitmap.Height != height)) {
            //				//Generate a thumbnail of the desired size
            //				int scale = (int)Math.Min(width/bitmap.Width,height/bitmap.Height);
            //				long weight = 0;
            //				byte * iter = (byte *)pData;
            //				byte val;
            //				Point size = PixelSize;
            //				LockBitmap();
            //	
            //				for(int column=0;column<bitmap.Width;column++) 
            //				{
            //					for(int row=0;row<bitmap.Height;row++) 
            //					{
            //						weight=0;
            //						for(int x=0; x < scale;x++) 
            //						{
            //							for(int y=0;y < scale;y++) 
            //							{
            //								weight += iter[(row*scale) + (y*width) + (column*scale) + x];
            //							}
            //						}
            //						if (weight!=0) System.Diagnostics.Debug.Assert(false);
            //						val = (byte)(weight/(scale*scale));
            //						PixelData * pPixel = PixelAt(row,column);
            //						pPixel->red   = val;
            //						pPixel->green = val;
            //						pPixel->blue  = val;
            //					}
            //				}
            //				UnlockBitmap();
            //			} else {
            //				byte * iter = (byte *)pData;
            //				Point size = PixelSize;
            //				LockBitmap();
            //				for (int y=0;y<height;y++) {
            //					PixelData * pPixel = PixelAt(0,y);
            //					for (int x=0;x < width;x++) {
            //						pPixel->red   = *iter;
            //						pPixel->green = *iter;
            //						pPixel->blue  = *iter;
            //						iter++;
            //						pPixel++;
            //					}
            //				}
            //				UnlockBitmap();
            //			} 
            #endregion
        }

        public void Resize(int newWidth, int newHeight)
        {
            Bitmap result = new Bitmap(newWidth, newHeight);
            using (Graphics g = Graphics.FromImage(result))
            {
                g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.HighQuality;
                g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                g.PixelOffsetMode = PixelOffsetMode.HighQuality;
                g.DrawImage(this.StoredBitmap, 0, 0, newWidth, newHeight);
                this.bitmap = result;
            }
            return;

        }
        public void ChangeImageOpacity(float opacityLevel)
        {
            if ((this.bitmap.PixelFormat & PixelFormat.Indexed) == PixelFormat.Indexed)
            {
                // Cannot modify an image with indexed colors
                return;
            }

            Bitmap bmp = (Bitmap)this.bitmap.Clone();
            // Specify a pixel format.
            PixelFormat pxf = PixelFormat.Format32bppArgb;

            // Lock the bitmap's bits.
            Rectangle rect = new Rectangle(0, 0, bmp.Width, bmp.Height);
            BitmapData bmpData = bmp.LockBits(rect, ImageLockMode.ReadWrite, pxf);

            // Get the address of the first line.
            IntPtr ptr = bmpData.Scan0;
            // Declare an array to hold the bytes of the bitmap.
            // This code is specific to a bitmap with 32 bits per pixels 
            // (32 bits = 4 bytes, 3 for RGB and 1 byte for alpha).
            int numBytes = bmp.Width * bmp.Height * bytesPerPixel;
            byte[] argbValues = new byte[numBytes];

            // Copy the ARGB values into the array.
            System.Runtime.InteropServices.Marshal.Copy(ptr, argbValues, 0, numBytes);

            // Manipulate the bitmap, such as changing the
            // RGB values for all pixels in the the bitmap.
            for (int counter = 0; counter < argbValues.Length; counter += bytesPerPixel)
            {
                // argbValues is in format BGRA (Blue, Green, Red, Alpha)

                // If 100% transparent, skip pixel
                if (argbValues[counter + bytesPerPixel - 1] == 0)
                    continue;

                int pos = 0;
                pos++; // B value
                pos++; // G value
                pos++; // R value

                argbValues[counter + pos] = (byte)(argbValues[counter + pos] * opacityLevel);
            }

            // Copy the ARGB values back to the bitmap
            System.Runtime.InteropServices.Marshal.Copy(argbValues, 0, ptr, numBytes);
            
            this.bitmap.Dispose();
            // Unlock the bits
            bmp.UnlockBits(bmpData);
            bmp.MakeTransparent(bmp.GetPixel(1, 1));
            this.bitmap = (Bitmap)bmp.Clone();
            bmp.Dispose();
            return;

        }
        //public void Resize(int newWidth, int newHeight)
        //{
        //    Bitmap result = new Bitmap(newWidth, newHeight);
        //    using (Graphics g = Graphics.FromImage(result))
        //    {
        //        g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.HighQuality;
        //        g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
        //        g.PixelOffsetMode = PixelOffsetMode.HighQuality;
        //        g.DrawImage(this.StoredBitmap, 0, 0, newWidth, newHeight);
        //        this.bitmap = result;
        //    }
        //    return;

        //}
        #endregion

        public void MakeGrey()
        {
            if (!isRGBImage) return;
            Point size = PixelSize;
            LockBitmap();
            for (int y = 0; y < size.Y; y++)
            {
                PixelData* pPixel = PixelAt(0, y);
                for (int x = 0; x < size.X; x++)
                {
                    byte value = (byte)((pPixel->red + pPixel->green + pPixel->blue) / 3);
                    pPixel->red = value;
                    pPixel->green = value;
                    pPixel->blue = value;
                    pPixel++;
                }
            }
            UnlockBitmap();
        }

        private void SetGrayscalePalette()
        {
            if (bitmap == null) return;
            ColorPalette pal = bitmap.Palette;
            for (int i = 0; i < 256; i++)
                pal.Entries[i] = Color.FromArgb(255, i, i, i);
            bitmap.Palette = pal;
        }

    }
}

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using DotNetMatrix;

namespace ImageUtils
{
    /// <summary>
    /// Helper functions to map DICOM CS to UI(.NET) CS
    /// </summary>
    public class CoordinateMapping
    {
        private GeneralMatrix m_transform;

        public CoordinateMapping()
        {
            m_transform = GeneralMatrix.Identity(3, 3);
        }

        public GeneralMatrix Transform
        {
            get { return m_transform; }
        }

        public void Clear()
        {
            m_transform = GeneralMatrix.Identity(3, 3);
        }

        public Point GetActualPosition(Point p)
        {
            GeneralMatrix mat = new GeneralMatrix(3, 1);
            mat.SetElement(0, 0, (double)p.X);
            mat.SetElement(1, 0, (double)p.Y);
            mat.SetElement(2, 0, 1.0);
            GeneralMatrix ret = m_transform.Inverse().Multiply(mat);
            return new Point((int)ret.GetElement(0, 0), (int)ret.GetElement(1, 0));
        }

        public Point GetActualDisplayPosition(Point p)
        {
            GeneralMatrix mat = new GeneralMatrix(3, 1);
            mat.SetElement(0, 0, (double)p.X);
            mat.SetElement(1, 0, (double)p.Y);
            mat.SetElement(2, 0, 1.0);
            GeneralMatrix ret = m_transform.Multiply(mat);
            return new Point((int)ret.GetElement(0, 0), (int)ret.GetElement(1, 0));
        }

        public PointF GetActualDisplayPosition(PointF p)
        {
            GeneralMatrix mat = new GeneralMatrix(3, 1);
            mat.SetElement(0, 0, p.X);
            mat.SetElement(1, 0, p.Y);
            mat.SetElement(2, 0, 1.0);
            GeneralMatrix ret = m_transform.Multiply(mat);
            return new PointF((float)ret.GetElement(0, 0), (float)ret.GetElement(1, 0));
        }

        public Point RotatePoint(Point pointToRotate, Point centerPoint, int angleInDegrees)
        {
            double angleInRadians = angleInDegrees * (Math.PI / 180);
            double cosTheta = Math.Cos(angleInRadians);
            double sinTheta = Math.Sin(angleInRadians);
            int x = (int)
                    (cosTheta * (pointToRotate.X - centerPoint.X) -
                    sinTheta * (pointToRotate.Y - centerPoint.Y) + centerPoint.X);
            int y = (int)
                    (sinTheta * (pointToRotate.X - centerPoint.X) +
                    cosTheta * (pointToRotate.Y - centerPoint.Y) + centerPoint.Y);
            return new Point(x, y);
        }

        public PointF RotatePoint(PointF pointToRotate, PointF centerPoint, int angleInDegrees)
        {
            double angleInRadians = angleInDegrees * (Math.PI / 180);
            double cosTheta = Math.Cos(angleInRadians);
            double sinTheta = Math.Sin(angleInRadians);
            float x = (float)(cosTheta * (pointToRotate.X - centerPoint.X) -
                    sinTheta * (pointToRotate.Y - centerPoint.Y) + centerPoint.X);
            float y = (float)
                    (sinTheta * (pointToRotate.X - centerPoint.X) +
                    cosTheta * (pointToRotate.Y - centerPoint.Y) + centerPoint.Y);
            return new PointF(x, y);
        }
    }
}

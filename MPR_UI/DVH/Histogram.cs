using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using DevExpress.XtraCharts;
using DevExpress.Utils;

namespace MPR_UI.DVH
{
    public partial class Histogram : UserControl
    {
        private ChartControl m_chart;
        public Histogram()
        {
            InitializeComponent();
            this.Load += Histogram_Load;
        }
        #region Properties
        ChartControl Chart { get { return m_chart; } }
        XYDiagram Diagram { get { return m_chart.Diagram as XYDiagram; } }

        AxisX DoseCGYAxis { get { return Diagram.AxisX; } }
        AxisY VolumeAxis { get { return Diagram.AxisY; } }
        SecondaryAxisX DosePercentageAxis { get { return Diagram.SecondaryAxesX[0]; } set { Diagram.SecondaryAxesX.Add(value); } }

        XYDiagramPaneBase DiagramPane { get { return Diagram.DefaultPane; } }
        #endregion
        void Histogram_Load(object sender, EventArgs e)
        {
            m_chart = new ChartControl();
            m_chart.BackColor = Color.Black;
            m_chart.Dock = DockStyle.Fill;


            m_chart.CrosshairOptions.ShowCrosshairLabels = false;
            m_chart.CrosshairEnabled = DefaultBoolean.True;
            m_chart.CrosshairOptions.ShowArgumentLabels = true;
            m_chart.CrosshairOptions.ShowValueLabels = true;

            m_chart.CrosshairOptions.ShowValueLine = true;
            m_chart.CrosshairOptions.ShowArgumentLine = true;
            m_chart.CrosshairOptions.CrosshairLabelMode = CrosshairLabelMode.ShowForNearestSeries;
            m_chart.CrosshairOptions.ArgumentLineColor = Color.White;
            m_chart.CrosshairOptions.ArgumentLineStyle.DashStyle = DashStyle.Dash;
            m_chart.CrosshairOptions.ValueLineColor = Color.White;
            m_chart.CrosshairOptions.ValueLineStyle.DashStyle = DashStyle.Dash;

            // add a fake series
            {
                Series fakeSeries = new Series();
                fakeSeries.LabelsVisibility = DefaultBoolean.False;
                fakeSeries.ShowInLegend = false;
                m_chart.Series.Add(fakeSeries);
            }
            m_chart.BackColor = Color.Black;

            //// customize chart axes
            CustomizeAxis(DoseCGYAxis, "Dose(cGy)");
            CustomizeAxis(VolumeAxis, "Volume(%)");

            //// Create  secondary axes, and add them to the chart's Diagram.
            DosePercentageAxis = new SecondaryAxisX("Dose(%)");

            CustomizeAxis(DosePercentageAxis, "Dose(%)");

            Diagram.DefaultPane.BorderColor = Color.White;
            Diagram.DefaultPane.BackColor = Color.Black;

            DoseCGYAxis.CrosshairAxisLabelOptions.Pattern = "{A:F1}";
            VolumeAxis.CrosshairAxisLabelOptions.Pattern = "{V:F1}";
            DosePercentageAxis.CrosshairAxisLabelOptions.Pattern = "{A:F1}";

            DoseCGYAxis.CrosshairAxisLabelOptions.TextColor = Color.Black;
            VolumeAxis.CrosshairAxisLabelOptions.TextColor = Color.Black;
            DosePercentageAxis.CrosshairAxisLabelOptions.TextColor = Color.Black;

            DoseCGYAxis.CrosshairAxisLabelOptions.BackColor = Color.White;
            VolumeAxis.CrosshairAxisLabelOptions.BackColor = Color.White;
            DosePercentageAxis.CrosshairAxisLabelOptions.BackColor = Color.White;

            // Enable the X-axis zooming at the diagram's level.
            Diagram.EnableAxisXZooming = true;
            Diagram.EnableAxisYZooming = true;

            this.panel1.Controls.Add(m_chart);
        }

        private void CustomizeAxis(DevExpress.XtraCharts.Axis p_axis, string p_title)
        {
            p_axis.Color = Color.White;
            p_axis.Title.Text = p_title;
            p_axis.Title.Font = new System.Drawing.Font("Tahoma", 10.0F);
            p_axis.Title.TextColor = Color.White;
            p_axis.Title.Visible = true;
            p_axis.Title.Antialiasing = true;
            p_axis.Range.MinValue = 0;
            p_axis.Range.MaxValue = 100;
            p_axis.Label.TextColor = Color.White;
            p_axis.Tickmarks.MinorVisible = true;

            //p_axis.Alignment = AxisAlignment.Far;
            p_axis.Title.Alignment = StringAlignment.Far;
        }


        private Series CreateSeries(string p_name, int r, int g, int b)
        {
            Series _series = new Series(p_name, ViewType.Spline);
            _series.CrosshairEnabled = DefaultBoolean.True;
            // _series.CrosshairHighlightPoints = DefaultBoolean.True;
            ((SplineSeriesView)_series.View).LineStyle.Thickness = 2;
            ((SplineSeriesView)_series.View).LineTensionPercent = 90;
            _series.View.Color = Color.FromArgb(r, g, b);
            _series.ShowInLegend = true;

            return _series;
        }

        private Series GetSeries(string name)
        {
            foreach (Series eachSeries in Chart.Series)
            {
                if (eachSeries.Name == name)
                    return Chart.Series[name];
            }
            return null;

        }

        internal void AddSeries(string name, int r, int g, int b, List<double> p_DVHData, double maxDVHValue, double volume, double maxDose, double minDose, double meanDose, int targetPrescribedDose)
        {

            if (GetSeries(name) != null) /// Series with this name already exists.
            {
                return;
            }

            Series _series = CreateSeries(name, r, g, b); // create series
            int numberOfPoints = p_DVHData.Count;

            for (int i = 0; i < numberOfPoints; i++)
            {
                SeriesPoint tPoint = new SeriesPoint(i, p_DVHData[i]);
                _series.Points.Add(tPoint);

            }

            Chart.Series.Add(_series); // add series to chart.

            if ((double.IsNaN((double)((XYDiagram)m_chart.Diagram).AxisX.Range.MaxValue)) || (Convert.ToInt32(((XYDiagram)m_chart.Diagram).AxisX.Range.MaxValue)) < p_DVHData.Count)
            {
                ((XYDiagram)m_chart.Diagram).AxisX.Range.MaxValue = p_DVHData.Count;
            }


            if (Convert.ToInt32(Diagram.AxisX.Range.MaxValue) < p_DVHData.Count)
            {
                DoseCGYAxis.Range.MaxValue = p_DVHData.Count;
            }

            /// set range for secondary X-Axis which represents percentage of dose recieved relative to Dose cGy value.
            DosePercentageAxis.Range.MaxValue = Convert.ToDecimal(DoseCGYAxis.Range.MaxValue) / targetPrescribedDose * 100;

            //dataTable1.Rows.Add(name, Math.Round(volume, 1), Math.Round(maxDose, 1), Math.Round(minDose, 1), Math.Round(meanDose, 1), true);
        }

        internal void Clear()
        {
            
            // int j = Chart.Series.Count;
            //while(j>0)
            //{
            //    Chart.Series.Remove(Chart.Series[j]);
            //    j--;
            //}

        }

        internal void AddPoints(List<double> values)
        {
            
        }

        internal void SetUpCCGraph()
        {
            VolumeAxis.Title.Text = "Volume (CC)";
        }

        internal void SetUpPercentageGraph()
        {
            VolumeAxis.Title.Text = "Volume (%)";
        }

        internal void AddComputeSeries(string p1, int p2, int p3, int p4, List<double> list, double maxVal, int p5, int p6, int p7, int p8, short p9)
        {
            AddSeries(p1, p2, p3, p4, list, maxVal, p5, p6, p7, p8, p9);
            Series s = this.GetSeries(p1);
            ((SplineSeriesView)s.View).LineStyle.DashStyle = DashStyle.Dash;
        }
    }
}

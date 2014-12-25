using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using ImageUtils;
using MPR_UI.Properties;
using MPR_VTK_BRIDGE;

namespace MPR_UI
{
    [System.Runtime.InteropServices.GuidAttribute("063C04C5-9EA1-45A2-AE52-2AEB854437CE")]
    public partial class MainForm : Form
    {
        private ImageControl axialImage;
        private ImageControl coronalImage;
        private ImageControl sagittalImage;
        private DVHControl dvhControl;

        private TableLayoutPanel tableLayout1x1;
        private TableLayoutPanel tableLayout2x2;

        public MainForm()
        {
            InitializeComponent();

            axialImage = new ImageControl(Axis.AxialAxis);
            axialImage.Dock = DockStyle.Fill;
            sagittalImage = new ImageControl(Axis.SagittalAxis);
            sagittalImage.Dock = DockStyle.Fill;
            coronalImage = new ImageControl(Axis.CoronalAxis);
            coronalImage.Dock = DockStyle.Fill;
            dvhControl = new DVHControl();
            dvhControl.Dock = DockStyle.Fill;

            this.tableLayout1x1 = new TableLayoutPanel();
            this.tableLayout1x1.Dock = DockStyle.Fill;
            this.tableLayout1x1.RowCount = 1;
            this.tableLayout1x1.ColumnCount = 1;

            this.tableLayout1x1.RowStyles.Clear();
            this.tableLayout1x1.ColumnStyles.Clear();

            for (int i = 0; i < this.tableLayout1x1.RowCount; i++)
            {
                this.tableLayout1x1.RowStyles.Add(new RowStyle(SizeType.Percent, 100 / this.tableLayout1x1.RowCount));
            }
            for (int i = 0; i < this.tableLayout1x1.ColumnCount; i++)
            {
                this.tableLayout1x1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100 / this.tableLayout1x1.ColumnCount));
            }
            this.tableLayout2x2 = new TableLayoutPanel();
            this.tableLayout2x2.Dock = DockStyle.Fill;
            this.tableLayout2x2.RowCount = 2;
            this.tableLayout2x2.ColumnCount = 2;
            this.tableLayout2x2.CellBorderStyle = TableLayoutPanelCellBorderStyle.Single;
            this.tableLayout2x2.RowStyles.Clear();
            this.tableLayout2x2.ColumnStyles.Clear();
            for (int i = 0; i < this.tableLayout2x2.RowCount; i++)
            {
                this.tableLayout2x2.RowStyles.Add(new RowStyle(SizeType.Percent, 100 / this.tableLayout2x2.RowCount));
            }
            for (int i = 0; i < this.tableLayout2x2.ColumnCount; i++)
            {
                this.tableLayout2x2.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100 / this.tableLayout2x2.ColumnCount));
            }
            this.panel1.Controls.Add(tableLayout2x2);
            this.Load += Form1_Load;
            
        }

        void Restore2x2Layout()
        {
            tableLayout2x2.SuspendLayout();

            

            tableLayout2x2.Controls.Add(axialImage, 0, 0);

            
            tableLayout2x2.Controls.Add(sagittalImage, 1, 0);

            
            tableLayout2x2.Controls.Add(coronalImage, 0, 1);

           
            tableLayout2x2.Controls.Add(dvhControl, 1, 1);


            tableLayout2x2.ResumeLayout();
        }
        void Form1_Load(object sender, EventArgs e)
        {
            EventHandling.EventHandler1.Instance.EVT_PanelResizing += Instance_EVT_PanelResizing;
            Restore2x2Layout();
            
            MessageBox.Show("Done");
        }

        void Instance_EVT_PanelResizing(object sender, bool state)
        {
            if(sender.GetType().Name == "ImageControl")
            {
                var imgControl = sender as ImageControl;
                switch(imgControl.Axis)
                {
                    case Axis.AxialAxis:
                        imgControl = axialImage;
                        break;
                    case Axis.CoronalAxis:
                        imgControl = coronalImage;

                        break;
                    case Axis.SagittalAxis:
                        imgControl = sagittalImage;
                        break;
                }
                if (state)
                {
                    this.panel1.Controls.Clear();
                    tableLayout1x1.Controls.Clear();
                    tableLayout1x1.Controls.Add(imgControl, 0, 0);
                    this.panel1.Controls.Add(tableLayout1x1);

                }
                else
                {
                    this.panel1.Controls.Clear();
                    Restore2x2Layout();
                    //tableLayout1x1.Controls.Clear();
                    //tableLayout1x1.Controls.Add(imgControl, 0, 0);
                    this.panel1.Controls.Add(tableLayout2x2);

                    //tableLayoutPanel1.SetRowSpan(imgControl,1);
                    //tableLayoutPanel1.SetColumnSpan(imgControl, 1);
                }
            }
            else
            {
                MessageBox.Show("Unhandled type:{0}",sender.GetType().Name);
            }
        }

        private void MainForm_Resize(object sender, EventArgs e)
        {
            
        }

        private void button1_Click(object sender, EventArgs e)
        {
            button1.Enabled = false;
            MPR_UI_Interface ui_interface = MPR_UI_Interface.GetHandle();
            ui_interface.InitMPR(Settings.Default.DICOM_DIR);
            axialImage.InitScrollBarAndLoadImage();
            sagittalImage.InitScrollBarAndLoadImage();
            coronalImage.InitScrollBarAndLoadImage();
        }
    }
}

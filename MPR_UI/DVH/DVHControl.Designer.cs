namespace MPR_UI
{
    partial class DVHControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.splashScreenManager = new DevExpress.XtraSplashScreen.SplashScreenManager(this, typeof(global::MPR_UI.DVH.DVHLoading), true, true, DevExpress.XtraSplashScreen.ParentType.UserControl);
            this.generateDVH_BTN = new System.Windows.Forms.Button();
            this.panel1 = new System.Windows.Forms.Panel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.percentView_BTN = new System.Windows.Forms.Button();
            this.ccView_BTN = new System.Windows.Forms.Button();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // generateDVH_BTN
            // 
            this.generateDVH_BTN.Dock = System.Windows.Forms.DockStyle.Fill;
            this.generateDVH_BTN.Location = new System.Drawing.Point(3, 297);
            this.generateDVH_BTN.Name = "generateDVH_BTN";
            this.generateDVH_BTN.Size = new System.Drawing.Size(324, 24);
            this.generateDVH_BTN.TabIndex = 0;
            this.generateDVH_BTN.Text = "Generate DVH";
            this.generateDVH_BTN.UseVisualStyleBackColor = true;
            this.generateDVH_BTN.Click += new System.EventHandler(this.generateDVH_BTN_Click);
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.SystemColors.Desktop;
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(3, 38);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(324, 253);
            this.panel1.TabIndex = 1;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.percentView_BTN);
            this.flowLayoutPanel1.Controls.Add(this.ccView_BTN);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(3, 3);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(324, 29);
            this.flowLayoutPanel1.TabIndex = 2;
            // 
            // percentView_BTN
            // 
            this.percentView_BTN.Location = new System.Drawing.Point(3, 3);
            this.percentView_BTN.Name = "percentView_BTN";
            this.percentView_BTN.Size = new System.Drawing.Size(75, 23);
            this.percentView_BTN.TabIndex = 0;
            this.percentView_BTN.Text = "button1";
            this.percentView_BTN.UseVisualStyleBackColor = true;
            this.percentView_BTN.Click += new System.EventHandler(this.percentView_BTN_Click);
            // 
            // ccView_BTN
            // 
            this.ccView_BTN.Location = new System.Drawing.Point(84, 3);
            this.ccView_BTN.Name = "ccView_BTN";
            this.ccView_BTN.Size = new System.Drawing.Size(75, 23);
            this.ccView_BTN.TabIndex = 1;
            this.ccView_BTN.Text = "button2";
            this.ccView_BTN.UseVisualStyleBackColor = true;
            this.ccView_BTN.Click += new System.EventHandler(this.ccView_BTN_Click);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel1, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.generateDVH_BTN, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.panel1, 0, 1);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 3;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 35F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 30F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(330, 324);
            this.tableLayoutPanel1.TabIndex = 3;
            // 
            // DVHControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tableLayoutPanel1);
            this.Name = "DVHControl";
            this.Size = new System.Drawing.Size(330, 324);
            this.Controls.SetChildIndex(this.tableLayoutPanel1, 0);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button generateDVH_BTN;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.Button percentView_BTN;
        private System.Windows.Forms.Button ccView_BTN;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private DevExpress.XtraSplashScreen.SplashScreenManager splashScreenManager;


    }
}

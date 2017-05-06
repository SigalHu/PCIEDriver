namespace PCIEDriverTest
{
    partial class DmaTransfer
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DmaTransfer));
            this.pgbDmaTransfer = new System.Windows.Forms.ProgressBar();
            this.label1 = new System.Windows.Forms.Label();
            this.lalDmaTransfer = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // pgbDmaTransfer
            // 
            this.pgbDmaTransfer.Location = new System.Drawing.Point(12, 60);
            this.pgbDmaTransfer.Name = "pgbDmaTransfer";
            this.pgbDmaTransfer.Size = new System.Drawing.Size(366, 23);
            this.pgbDmaTransfer.Step = 1;
            this.pgbDmaTransfer.TabIndex = 0;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("SimSun", 10.8F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.label1.Location = new System.Drawing.Point(12, 23);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(95, 19);
            this.label1.TabIndex = 1;
            this.label1.Text = "传输速度:";
            // 
            // lalDmaTransfer
            // 
            this.lalDmaTransfer.AutoSize = true;
            this.lalDmaTransfer.Font = new System.Drawing.Font("Calibri", 10.8F);
            this.lalDmaTransfer.Location = new System.Drawing.Point(112, 21);
            this.lalDmaTransfer.Name = "lalDmaTransfer";
            this.lalDmaTransfer.Size = new System.Drawing.Size(65, 23);
            this.lalDmaTransfer.TabIndex = 2;
            this.lalDmaTransfer.Text = "0 MB/s";
            // 
            // DmaTransfer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 18F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(255)))), ((int)(((byte)(192)))));
            this.ClientSize = new System.Drawing.Size(388, 98);
            this.ControlBox = false;
            this.Controls.Add(this.lalDmaTransfer);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.pgbDmaTransfer);
            this.Font = new System.Drawing.Font("SimSun", 10.8F);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DmaTransfer";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "DMA传输";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ProgressBar pgbDmaTransfer;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lalDmaTransfer;
    }
}
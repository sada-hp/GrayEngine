using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Runtime.InteropServices;

namespace EditorUI
{
    public partial class SkyboxSettings : Window
    {
        public SkyboxSettings()
        {
            InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            Preview_North.Source = null;
            Preview_South.Source = null;
            Preview_East.Source = null;
            Preview_West.Source = null;
            Preview_Top.Source = null;
            Preview_Bottom.Source = null;
            Preview_North.UpdateLayout();
            Preview_South.UpdateLayout();
            Preview_East.UpdateLayout();
            Preview_West.UpdateLayout();
            Preview_Top.UpdateLayout();
            Preview_Bottom.UpdateLayout();
            UIBridge.UpdateSkybox(Marshal.StringToHGlobalAnsi(East.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(West.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(Top.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(Bottom.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(North.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(South.ToolTip.ToString()));
            Close();
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true);
        }

        private ImageSource BitmapFromUri(Uri src, Rotation rot)
        {
            var btm = new BitmapImage();
            btm.BeginInit();
            btm.UriSource = src;
            btm.CacheOption = BitmapCacheOption.None;
            btm.Rotation = rot;
            btm.EndInit();
            btm.Freeze();
            
            return btm;
        }

        private void Load_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog();
            openFileDialog.Filter = "image files (*.png)|*.png";
            openFileDialog.RestoreDirectory = true;

            if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                (sender as System.Windows.Controls.Button).ToolTip = openFileDialog.FileName;
                (sender as System.Windows.Controls.Button).Background = null;
                (sender as System.Windows.Controls.Button).UpdateLayout();

                if (sender == North)
                {
                    Preview_North.Source = BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate180);
                }
                else if (sender == South)
                {
                    Preview_South.Source = BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0);
                }
                else if (sender == Top)
                {
                    Preview_Top.Source = BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate180);
                }
                else if (sender == Bottom)
                {
                    Preview_Bottom.Source = BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate0);
                }
                else if (sender == East)
                {
                    Preview_East.Source = BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate90);
                }
                else if (sender == West)
                {
                    Preview_West.Source = BitmapFromUri(new Uri(openFileDialog.FileName), Rotation.Rotate270);
                }
            }

            GC.Collect();
        }

        private void Cancel_Click(object sender, RoutedEventArgs e)
        {
            Close();
            GC.Collect();
        }
    }
}

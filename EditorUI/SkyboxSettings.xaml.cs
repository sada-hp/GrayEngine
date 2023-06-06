using System;
using System.Windows.Controls;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Runtime.InteropServices;
using System.Windows.Input;

namespace EditorUI
{
    public partial class SkyboxSettings : Window
    {
        bool ready = false;
        public SkyboxSettings()
        {
            InitializeComponent();
            ready = true;

            UpdateProps();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {

            UIBridge.UpdateSkybox(Marshal.StringToHGlobalAnsi(East.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(West.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(Top.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(Bottom.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(North.ToolTip.ToString()),
                Marshal.StringToHGlobalAnsi(South.ToolTip.ToString()));

            if ((sender as System.Windows.Controls.Button).Content.ToString() == "Confirm")
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
                Close();
            }
            GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true);
        }

        private void UpdateProps()
        {
            string color = Marshal.PtrToStringAnsi(UIBridge.GetSkyColor());
            string[] parse_info = color.Split(':');

            for (int i = 0; i < parse_info.Length; i++)
            {
                double value = 1.0;
                bool res = double.TryParse(parse_info[i], System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture.NumberFormat, out value);

                switch (i)
                {
                    case 0:
                        SliderRed.Value = value * 255;
                        break;
                    case 1:
                        SliderGreen.Value = value * 255;
                        break;
                    case 2:
                        SliderBlue.Value = value * 255;
                        break;
                    default:
                        continue;
                }
            }

            RedBox.Text = ((int)SliderRed.Value).ToString();
            GreenBox.Text = ((int)SliderGreen.Value).ToString();
            BlueBox.Text = ((int)SliderBlue.Value).ToString();
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

        private void Slider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            UpdateColor();

            if ((sender as Slider).IsFocused)
            {
                UpdateText();
            }
        }

        private void UpdateColor()
        {
            if (!ready) return;

            float r = (float)(SliderRed.Value / 255.0 * SliderBrightness.Value);
            float g = (float)(SliderGreen.Value / 255.0 * SliderBrightness.Value);
            float b = (float)(SliderBlue.Value / 255.0 * SliderBrightness.Value);
            float a = 1;
            var brush = new System.Windows.Media.LinearGradientBrush(System.Windows.Media.Color.FromRgb((byte)0, (byte)0, (byte)0), System.Windows.Media.Color.FromRgb((byte)SliderRed.Value, (byte)SliderGreen.Value, (byte)SliderBlue.Value), new Point(0, 0.5), new Point(1, 0.5));
            ColorPreview.Background = brush;
            UIBridge.SetSkyColor(r, g, b);
        }

        private void UpdateText()
        {
            if (!ready) return;

            RedBox.Text = ((int)SliderRed.Value).ToString();
            GreenBox.Text = ((int)SliderGreen.Value).ToString();
            BlueBox.Text = ((int)SliderBlue.Value).ToString();
        }

        private void TextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            System.Text.RegularExpressions.Regex reg = new System.Text.RegularExpressions.Regex("[0-9]");
            if (!reg.IsMatch(e.Text))
            {
                e.Handled = true;
            }
        }

        private void RedBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (!RedBox.IsKeyboardFocused) return;

            int val;
            if (int.TryParse(RedBox.Text, out val))
            {
                SliderRed.Value = val;
            }
        }

        private void GreenBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (!GreenBox.IsKeyboardFocused) return;

            int val;
            if (int.TryParse(GreenBox.Text, out val))
            {
                SliderGreen.Value = val;
            }
        }

        private void BlueBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (!BlueBox.IsKeyboardFocused) return;

            int val;
            if (int.TryParse(BlueBox.Text, out val))
            {
                SliderBlue.Value = val;
            }
        }
    }
}
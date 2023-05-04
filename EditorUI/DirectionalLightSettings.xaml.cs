using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Runtime.InteropServices;

namespace EditorUI
{
    public partial class DirectionalLightSettings : Window
    {
        bool mouse_down = false;
        bool mouse_down2 = false;
        double pitch = 0.0;
        double yaw = 0.0;
        public DirectionalLightSettings()
        {
            InitializeComponent();

            if (UIBridge.CheckCascade() == false)
            {
                Tab.IsEnabled = false;
                Tab.Visibility = Visibility.Hidden;
                Button btn = new Button();
                btn.Content = "Add directional light";
                btn.VerticalAlignment = VerticalAlignment.Center;
                btn.HorizontalAlignment = HorizontalAlignment.Center;
                btn.Width = 200;
                btn.Height = 75;
                btn.Background = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(93, 93, 93));
                btn.BorderBrush = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(200, 200, 200));
                btn.Foreground = new System.Windows.Media.SolidColorBrush(System.Windows.Media.Color.FromRgb(255, 255, 255));
                btn.BorderThickness = new Thickness(2);
                btn.FontFamily = new System.Windows.Media.FontFamily("Lucida Sans Unicode");
                btn.Cursor = Cursors.Hand;
                btn.Click += ButtoCreate;
                MainGrid.Children.Add(btn);
            }
            else
            {
                UpdateProps();
            }
        }

        private void UpdateProps()
        {
            string color = Marshal.PtrToStringAnsi(UIBridge.GetCascadeColor());
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
        }

        private void ButtoCreate(object sender, RoutedEventArgs e)
        {
            UIBridge.AddCascade();
            MainGrid.Children.Remove((sender as Button));
            Tab.IsEnabled = true;
            Tab.Visibility = Visibility.Visible;
            UpdateProps();
        }

        private void Ellipse_MouseDown(object sender, MouseButtonEventArgs e)
        {
            mouse_down = true;
        }

        private void Sun_MouseUp(object sender, MouseButtonEventArgs e)
        {
            mouse_down = false;
            mouse_down2 = false;
        }

        private void Angle(Point MousePos)
        {
            Point origin = WorldOrig.PointToScreen(new Point(WorldOrig.Width/2, WorldOrig.Height/2 - 5));
            Point canvas_right = SideCanvas.PointToScreen(new Point(450, SideCanvas.Height));
            Point canvas_left = SideCanvas.PointToScreen(new Point(0, SideCanvas.Height));
            Point vec1 = new Point(canvas_right.X- canvas_left.X, canvas_right.Y - canvas_left.Y);
            Point vec2 = new Point(MousePos.X - origin.X, MousePos.Y - origin.Y);
            double dot = vec1.X * vec2.X + vec1.Y * vec2.Y;
            double mag = Math.Sqrt(Math.Pow(vec1.X, 2) + Math.Pow(vec1.Y, 2)) * Math.Sqrt(Math.Pow(vec2.X, 2) + Math.Pow(vec2.Y, 2));
            double cos = dot/ mag;
            double sin = Math.Sqrt(1 - Math.Pow(cos, 2));
            double bot = sin;
            double left = cos;
            Canvas.SetLeft(Sun, 210 + 125 * left);
            Canvas.SetBottom(Sun, 110 * bot);
            pitch = 45 - (180 * Math.Acos(cos) / Math.PI);
            UIBridge.RotateSun((float)pitch, (float)yaw);
            //Title = (180 * Math.Acos(cos) / Math.PI).ToString();
        }

        private void Angle2(Point MousePos)
        {
            Point origin = TopOrigin.PointToScreen(new Point(TopOrigin.Width/2, TopOrigin.Height/2));
            Point canvas_right = TopCanvas.PointToScreen(new Point(450, TopCanvas.Height/2));
            Point canvas_left = TopCanvas.PointToScreen(new Point(0, TopCanvas.Height/2));
            Point vec1 = new Point(canvas_right.X - canvas_left.X, canvas_right.Y - canvas_left.Y);
            Point vec2 = new Point(MousePos.X - origin.X, MousePos.Y - origin.Y);
            int sign = 1;
            double offset = 0;
            if (MousePos.Y > origin.Y)
            {
                sign = -1;
                offset = Sun2.Height / 2;
            }
            double dot = vec1.X * vec2.X + vec1.Y * vec2.Y;
            double mag = Math.Sqrt(Math.Pow(vec1.X, 2) + Math.Pow(vec1.Y, 2)) * Math.Sqrt(Math.Pow(vec2.X, 2) + Math.Pow(vec2.Y, 2));
            double cos = dot / mag;
            double sin = Math.Sqrt(1 - Math.Pow(cos, 2));
            double bot = sin;
            double left = cos;
            Canvas.SetLeft(Sun2, 210 + 100 * left);
            Canvas.SetBottom(Sun2, 92.5 + 92.5 * bot * sign - offset);
            yaw = 180 * Math.Acos(cos) / Math.PI * sign;
            UIBridge.RotateSun((float)pitch, (float)yaw);
            //Title = (180 * Math.Acos(cos) / Math.PI).ToString();
        }

        private void Sun_MouseMove(object sender, MouseEventArgs e)
        {
            if (mouse_down == false)
            {
                return;
            }
            else
            {
                Angle(this.PointToScreen(e.GetPosition(this)));
            }
        }

        private void Path_MouseMove(object sender, MouseEventArgs e)
        {
            if (mouse_down == false)
            {
                return;
            }
            else
            {
                Angle(this.PointToScreen(e.GetPosition(this)));
            }
        }

        private void Window_MouseUp(object sender, MouseButtonEventArgs e)
        {
            mouse_down = false;
            mouse_down2 = false;
        }

        private void Sun2_MouseDown(object sender, MouseButtonEventArgs e)
        {
            mouse_down2 = true;
        }

        private void Ellipse_MouseMove(object sender, MouseEventArgs e)
        {
            if (mouse_down2 == false)
            {
                return;
            }
            else
            {
                Angle2(this.PointToScreen(e.GetPosition(this)));
            }
        }

        private void Sun2_MouseMove(object sender, MouseEventArgs e)
        {
            if (mouse_down2 == false)
            {
                return;
            }
            else
            {
                Angle2(this.PointToScreen(e.GetPosition(this)));
            }
        }

        private void Window_MouseMove(object sender, MouseEventArgs e)
        {
            if (mouse_down2)
            {
                Angle2(this.PointToScreen(e.GetPosition(this)));
            }
            else if (mouse_down)
            {
                Angle(this.PointToScreen(e.GetPosition(this)));
            }
        }

        private void Slider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            UpdateColor();
        }

        private void UpdateColor()
        {
            if (SliderRed == null || SliderGreen == null || SliderBlue == null || SliderBrightness == null || SliderAmbient == null) return;

            float r = (float)(SliderRed.Value / 255.0 * SliderBrightness.Value);
            float g = (float)(SliderGreen.Value / 255.0 * SliderBrightness.Value);
            float b = (float)(SliderBlue.Value / 255.0 * SliderBrightness.Value);
            float a = 1;
            var brush = new System.Windows.Media.LinearGradientBrush(System.Windows.Media.Color.FromRgb((byte)SliderAmbient.Value, (byte)SliderAmbient.Value, (byte)SliderAmbient.Value), System.Windows.Media.Color.FromRgb((byte)SliderRed.Value, (byte)SliderGreen.Value, (byte)SliderBlue.Value), new Point(0, 0.5), new Point(1, 0.5));
            ColorPreview.Background = brush;
            UIBridge.SetSunColor(Marshal.StringToHGlobalAnsi(r.ToString().Replace(',', '.') + ":" + g.ToString().Replace(',', '.') + ":" + b.ToString().Replace(',', '.') + ":" + a.ToString().Replace(',', '.')));
        }

        private void SliderAmbient_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            float r = (float)(SliderRed.Value / 255.0 * SliderBrightness.Value);
            float g = (float)(SliderGreen.Value / 255.0 * SliderBrightness.Value);
            float b = (float)(SliderBlue.Value / 255.0 * SliderBrightness.Value);
            float a = 1;
            var brush = new System.Windows.Media.LinearGradientBrush(System.Windows.Media.Color.FromRgb((byte)SliderAmbient.Value, (byte)SliderAmbient.Value, (byte)SliderAmbient.Value), System.Windows.Media.Color.FromRgb((byte)SliderRed.Value, (byte)SliderGreen.Value, (byte)SliderBlue.Value), new Point(0, 0.5), new Point(1, 0.5));
            ColorPreview.Background = brush;
            UIBridge.SetAmbientModulator((float)(SliderAmbient.Value / 255.0));
        }
    }
}

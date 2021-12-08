clear;
clc;
close all;

FontSize = 20;

% g in meters per second
g = 9.81;

% Read data from CSV file
data = csvread("accel_data.txt");
data = data * g;
ax = double(data(1:end,1));
ay = double(data(1:end,2));
az = double(data(1:end,3));

% Calculate magnitude of acceleration
a = sqrt(ax.^2 + ay.^2 + az.^2);


fs = 1000;  % Sampling frequency
fn = fs/2;  % Nyquist frequency

% Band pass filter frequencies [Hz]
fa =  0;
fb =  10.0;

% Normalized frequencies
fa_n = fa / fn; 
fb_n = fb / fn;

filter_order = 7;
c = fir1(filter_order, [fa_n, fb_n], "pass");

% Applay filter
ax_filter = filter(c, 1, ax);
ay_filter = filter(c, 1, ay);
az_filter = filter(c, 1, az);

% Calculate time step
h = 1 / fs;

% Create time vector
t = [0:1:length(az_filter)-1]' * h;

% Plot filtered data with grid
k = 250;
figure(1)
plot(t(1:k),ax(1:k),t(1:k), ax_filter(1:k), 'LineWidth',1)
hx = xlabel("t [s]");
hy = ylabel("g_x [°/s]");
ht = title("Gyroscope data - X-axis");
set (hx, "fontsize", FontSize);
set (hy, "fontsize", FontSize);
set (ht, "fontsize", FontSize);
legend("X-axes raw data", "X-axes filtered data")
grid on;

figure(2)
plot(t(1:k),ay(1:k),t(1:k), ay_filter(1:k), 'LineWidth',1)
hx = xlabel("t [s]");
hy = ylabel("a_y [m/s^2]");
ht = title("Accelerometer data - Y-axis");
set (hx, "fontsize", FontSize);
set (hy, "fontsize", FontSize);
set (ht, "fontsize", FontSize);
legend("Y-axis raw data", "Y-axis filtered data")
grid on;

figure(3)
plot(t(1:k),az(1:k),t(1:k), az_filter(1:k), 'LineWidth',1)
hx = xlabel("t [s]");
hy = ylabel("a_z [m/s^2]");
ht = title("Accelerometer data - Z-axis");
set (hx, "fontsize", FontSize);
set (hy, "fontsize", FontSize);
set (ht, "fontsize", FontSize);
legend("Z-axes raw data", "Z-axes filtered data")
grid on;


figure(4)
% SPECTRUM OF SIGNAL
  % BEFORE FILTERING
  n = length(ax);
  FT = fft(ax);

  f_az = abs(FT)(1:end/2);
  arg_az = 180/pi * arg(FT)(1:end/2);
  freq_vec = [0:1:(n/2-1)] * fn/n;

  semilogx(freq_vec(2:end), 20*log10(f_az(2:end)/sqrt(n)), 'LineWidth',1)
  hx = xlabel("f [Hz]");
  hy = ylabel("Signal magnitude [dB]");
  ht = title("Accelerometer data spectrum");
  set (hx, "fontsize", FontSize);
  set (hy, "fontsize", FontSize);
  set (ht, "fontsize", FontSize);
  ylim([-100 60])
  grid on
  print -dpng -color "before-filter.png"
  clf
  
close
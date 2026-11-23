format long

%% MEASURING THE MOMENT OF INERTIA OF THE FLYWHEEL

%% 
% measurements of flywheel

d_smallhole = 11.95; %distances in mm
length_left_to_edge = 86.2;
r = 0.001*(length_left_to_edge - (d_smallhole/2)); %distance in m
m = 0.194; %mass in kg
g = 9.81;

%% A) OSCILLATIONS PERPENDICULAR TO KNIFE LENGTH

% TWENTY OSCILLATIONS

a_20 = 16.56;
b_20 = 16.64;
c_20 = 16.63;

time_20 = [a_20; b_20; c_20];
time_20_div = time_20/20;

% THIRTY OSCILLATIONS

a_30 = 24.78;
b_30 = 24.92;
c_30 = 24.89;
d_30 = 24.84;

time_30 = [a_30; b_30; c_30; d_30];
time_30_div = time_30/30;

list = [time_20_div; time_30_div];
P1_max = max(list);
P1_min = min(list);
P1_avg = mean(list);
disp("P1 max")
disp(P1_max)
disp("P1 min")
disp(P1_min)
disp("P1 avg")
disp(P1_avg)

%% B) OSCILLATIONS ALONG THE KNIFE LENGTH

% TWENTY OSCILLATIONS
a_20 = 14.36;
b_20 = 14.45;
c_20 = 14.29;

time_20 = [a_20; b_20; c_20];
time_20_div = time_20/20;

% THIRTY OSCILLATIONS

a_30 = 21.36;
b_30 = 21.62;
c_30 = 21.64;

time_30 = [a_30; b_30; c_30];
time_30_div = time_30/30;

list = [time_20_div; time_30_div];
P2_max = max(list)
P2_min = min(list)
P2_avg = mean(list)

%% CALCULATE MOMENT OF INERTIA

I_p1_max = ((m*g*r*P1_max*P1_max)/(2*pi)^2) - m*(r^2);
I_p1_min = ((m*g*r*P1_min*P1_min)/(2*pi)^2) - m*(r^2);
I_p1_avg = ((m*g*r*P1_avg*P1_avg)/(2*pi)^2) - m*(r^2);
I_p2_max = ((m*g*r*P2_max*P2_max)/(2*pi)^2) - m*(r^2);
I_p2_min = ((m*g*r*P2_min*P2_min)/(2*pi)^2) - m*(r^2);
I_p2_avg = ((m*g*r*P2_avg*P2_avg)/(2*pi)^2) - m*(r^2);

disp('I_p1_max')
disp(I_p1_max)
disp('I_p1_min')
disp(I_p1_min)
disp('I_p1_avg')
disp(I_p1_avg)
disp('I_p2_max')
disp(I_p2_max)
disp('I_p2_min')
disp(I_p2_min)
disp('I_p2_avg')
disp(I_p2_avg)

disp(I_p2_avg/I_p1_avg)

error_p1 = ((I_p1_max - I_p1_min)/I_p1_avg)*50
error_p2 = ((I_p2_max - I_p2_min)/I_p2_avg)*50
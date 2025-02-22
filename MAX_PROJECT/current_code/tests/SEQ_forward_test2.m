addpath('../core')
addpath('../utils')

% 'jpwh_991'; 'fs_680_1'; 'ifiss_convdiff'; 'shifted_laplacian_1d';
% 'thermal_eq_diff'; 'laplacian_2d'; 'SPN'; 'sp1'; 'SPN_shift';
% 'sp1_shift'; 'sp5_shift'; 'sp3_shift';
matrix='thermal_eq_diff';

[H,rhs, u, precond, Prec]=fixed_point(matrix);

%% NUmerical setting

numer.eps=10^(-3);
numer.rich_it=300;

%% Statistical setting

stat.nwalks=2;
stat.max_step=20;
stat.adapt_walks=1;
stat.adapt_cutoff=1;
stat.walkcut=10^(-6);
stat.nchecks=2;
stat.varcut=0.5;
stat.vardiff=10^(-6);
dist=1;

%% Definition of the transitional probability
[P, cdf]=prob_forward(H, dist);

%% Preconditioning setting

fp.u=u; %reference solution
fp.H=H; %iteration matrix
fp.rhs=rhs;
fp.precond='diag';
%% MCSA forward method resolution

start=cputime;
[sol, rel_residual, VAR, DX, NWALKS, tally, iterations, reject]=SEQ_forward(fp, P, cdf, numer, stat);
finish=cputime;

for i=1:iterations
    figure()
    %set(gca, 'XScale', 'log') 
    set(gca, 'YScale', 'log') 
    for j=1:size(u,1)
        hold on
        loglog(VAR{i*j}, '-o');
    end
    hold off
end

save(strcat('../results/MCSA_forward2/MCSA_forward_test2_', matrix, '_p=', num2str(dist)))
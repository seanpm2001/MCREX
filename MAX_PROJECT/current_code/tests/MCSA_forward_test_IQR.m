addpath('../core')
addpath('../utils')

% 'jpwh_991'; 'fs_680_1'; 'ifiss_convdiff'; 'shifted_laplacian_1d';
% 'thermal_eq_diff';  'laplacian_2d'
matrix='ifiss_convdiff';

addpath(strcat('../utils/model_problems/', matrix))

if strcmp(matrix, 'simple')
    dimen=50;
    A=4*diag(ones(dimen,1)) - diag(ones(dimen-1,1),1) - diag(ones(dimen-1,1),-1);
    rhs=ones(dimen,1);
    u=A\rhs;
    
else
     [A, dimen, ~, ~] = mmread('A.mtx');
     rhs=mmread('b.mtx');
     u=mmread('x.mtx');
end
    
Prec=diag(diag(A));

H=eye(size(A))-Prec\A;
rhs=Prec\rhs;

%% NUmerical setting

numer.eps=10^(-3);
numer.rich_it=300;

%% Statistical setting

stat.nwalks=10;
stat.max_step=1000;
stat.adapt_walks=1;
stat.adapt_cutoff=1;
stat.walkcut=10^(-6);
stat.nchecks=1;
stat.vardiff=0.5;
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

%parjob=parpool('local');
[sol, rel_residual, IQR, DX, NWALKS, tally, iterations]=MCSA_forward_IQR(fp, P, cdf, numer, stat);
%delete(parjob);

finish=cputime;


save(strcat('../results/MCSA_forward_IQR/MCSA_forward_test_', matrix, '_p=', num2str(dist)))
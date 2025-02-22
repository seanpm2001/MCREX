%UNSTEADY_STEP_NAVIER solve Navier-Stokes problem in step domain
%   IFISS scriptfile: DJS; 27 May 2012
% Copyright (c) 2009 D.J. Silvester
clear variables
global pde 
fprintf('Unsteady flow in a step domain ...\n')
global viscosity
viscosity=default('viscosity parameter (default 1/50)',1/50);
%
%% initialize for time stepping iteration: compute Stokes solution
%% load assembled matrices
gohome
cd datafiles
load step_stokes_nobc.mat
%
%
pde=14; domain=3;
%% set parameters
fprintf('Discrete Saddle-Point DAE system ...\n')
tfinal = default('target time? (default 200)',200);
tol = default('accuracy tolerance? (default 1e-4)',1e-4);
nstar = default('averaging frequency? (default 10)',10);
vswitch = default('plot vorticity evolution? 1/0',0);
dtzero=1e-9; uzero=zeros(size(f));gzero=zeros(size(g));
%% compute solution
tstart = tic;
if qmethod>1, 
np=length(g); 
AxB='defaultAxB'; 
[DT,U,Udot,time] = stabtrNS(qmethod,xy,mv,bound,A,B,sparse(np,np),G,AxB,...
							uzero,dtzero,tfinal,tol,nstar,1);
else  
% call the specialized saddle point system solver 
AxB='colamdAxB';  
beta=1;   
if qmethod==1, 
	  beta=1/4;     % default parameter
end  
% scaled beta (mutiplied by the viscosity)   
beta=beta*viscosity;
[DT,U,Udot,time] = stabtrNS(qmethod,xy,ev,bound,A,B,beta*C,G,AxB,...
							uzero,dtzero,tfinal,tol,nstar,1);   
end
etoc=toc(tstart); fprintf('Integration took  %8.3e seconds\n\n',etoc)

save step_unsteadyflow.mat DT U Udot time viscosity
%
% visualise solution and hold the timestep sequence plot
marker = 'bo';
dttplot(DT,13,marker)
if vswitch==1
pause(3)
   if qmethod>1,
   step_unsteadyflowplot(qmethod,mv,U,time,By,Bx,G,xy,xyp,x,y,bound,0.1,167);
   else
   step_unsteadyflowplot(qmethod,ev,U,time,By,Bx,G,xy,xyp,x,y,bound,0.1,167);
   end
end
fprintf('\nTo generate snapshots of stationary streamlines run \n')
help step_unsteadyflowref
fprintf('To generate a streamline movie run \n')
help step_flowmovie

%%% compute divergence error
if qmethod>1
   error_div = q2div(xy,mv,[U(:,end);gzero]);	
else
   error_div = q1div(xy,ev,[U(:,end);gzero]);	 	
end
return

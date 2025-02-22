%UNSTEADY_NAVIER_TESTPROBLEM_PC sets up Examples T-NS1 to T-NS5
%   IFISS scriptfile: DJS; 9 May 2012. 
% Copyright (c) 2009 D.J. Silvester, H.C. Elman, A. Ramage 
gohome
clear variables, clear functions
fprintf('\nspecification of reference unsteady Navier-Stokes problem.\n')
fprintf('\nchoose specific example (default is step)');
fprintf('\n     2  Flow over a backward facing step')
fprintf('\n     3  Lid driven cavity');
fprintf('\n     5  Flow around a square obstruction\n');
sn = default('',2);
if  sn==2
system('copy .\stokes_flow\test_problems\backwardstep_flow.m .\stokes_flow\specific_flow.m');
system('copy .\stokes_flow\test_problems\backwardstep_bc.m .\stokes_flow\stream_bc.m');
   longstep_stokes, unsteady_step_navier
elseif sn==3
   lid_model=default('cavity type leaky/tight/regularised 1/2/3 (regularised)',3); 
   if lid_model ==3    
system('copy .\stokes_flow\test_problems\regcavity_flow.m .\stokes_flow\specific_flow.m');
   elseif lid_model ==2    
system('copy .\stokes_flow\test_problems\tightcavity_flow.m .\stokes_flow\specific_flow.m');
   else
system('copy .\stokes_flow\test_problems\leakycavity_flow.m .\stokes_flow\specific_flow.m');
   end
system('copy .\stokes_flow\test_problems\zero_bc.m .\stokes_flow\stream_bc.m');
   square_stokes, unsteady_navier
   elseif sn==5
system('copy .\stokes_flow\test_problems\obstacle_flow.m .\stokes_flow\specific_flow.m');
system('copy .\stokes_flow\test_problems\obstacle_bc.m .\stokes_flow\stream_bc.m');
    obstacle_stokes, unsteady_obstacle_navier
else
   error('reference problem datafile not found!')
end

%% fix for pcg/minres bug
%% AR 05/05/2005
There is a known bug in the Matlab 7.0.4.352 (R14) Service Pack 2
release version of the routine
/opt/matlab7/toolbox/matlab/sparfun/private/iterapp.m.
To fix this, the four occurences of 'strcomp' in this file have to be
replaced by 'isequal'. We have implemented this change here. The files
pcg.m, minres.m and iterchk.m are identical to the release versions
but are included here for reasons of access.


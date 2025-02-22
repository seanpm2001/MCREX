function [sol, rel_res, VAR, REL_RES, DX, NWALKS, tally, count, reject]=MCSA_forward2(fp, P, cdf, numer, stat)

VAR=[];
NWALKS=[];
DX=[];
REL_RES=[];
n_walks=stat.nwalks;
max_step=stat.max_step;
eps=numer.eps;
rich_it=numer.rich_it;
reject=cell(rich_it,1);


if ~ strcmp(fp.precond, 'alternating')

%     if ~ asyn_check(fp.H,1)
%         error('Iteration matrix does not have spectral radius less than 1');
%     end 

    display(strcat('Forward MCSA with', {' '},  fp.precond, ' preconditioning started'));

    H=fp.H;
    rhs=fp.rhs;
    sol=rand(size(H,1),1);

    %matrix to be used for the computation of the residual at each Richardson
    %iteration
    B=sparse((speye(size(H))-H));  
    
    r=rhs-B*sol;
    rel_residual=norm(r,2)/norm(rhs,2);
    REL_RES=[REL_RES rel residual];

    count=1;

    if stat.adapt_walks==1 || stat.adapt_cutoff==1
        while(rel_residual>eps && count<=rich_it)
            display(strcat('iteration number', {' '}, num2str(count)))
            sol=sol+r; 
            r=rhs-B*sol;
            
            if count>2 && R(end)>R(end-1)
                stat.vardiff=stat.vardiff/2;
            end
            
            [dx, dvar, nwalks, rej]=MC_forward_adapt2(H, r, P, cdf, stat);
            NWALKS=[NWALKS nwalks]; 
            reject{count}=rej;
            sol=sol+dx;
            r=rhs-B*sol;
            display(strcat('residual norm: ', num2str(norm(r)/norm(rhs))));    
            rel_residual=norm(r,2)/norm(rhs,2);
            REL_RES=[REL_RES rel_residual];            
            VAR=[VAR dvar];
            DX=[DX dx];
            count=count+1;
        end
    else
        while(rel_residual>eps && count<=rich_it)
            display(strcat('iteration number', {' '}, num2str(count)))
            sol=sol+r; 
            r=rhs-B*sol;
            [dx, dvar, X]=MC_forward(H, r, P, cdf, n_walks, max_step);
            NWALKS=[NWALKS size(X,1)];
            sol=sol+dx;
            r=rhs-B*sol;
            display(strcat('residual norm: ', num2str(norm(r)/norm(rhs))));    
            rel_residual=norm(r,2)/norm(rhs,2);
            REL_RES=[REL_RES rel_residual];  
            VAR=[VAR dvar];
            DX=[DX dx];
            count=count+1;
        end
    end

    count=count-1;
    rel_res=rel_residual;


else

    if ~ asyn_check(fp.H1)
        error('Iteration matrix H1 does not have spectral radius less than 1');
    end

    if ~ asyn_check(fp.H2)
        error('Iteration matrix H2 does not have spectral radius less than 1');
    end

    display('Forward MCSA with alternating method started');

    H1=fp.H1;
    H2=fp.H2;
    rhs1=fp.rhs1;
    rhs2=fp.rhs2;

    sol=ones(size(H1,1),1);

    VAR1=[];
    VAR2=[];

    %matrix to be used for the computation of the redisual at each Richardson
    %iteration
    B1=(eye(size(H1))-H1);
    B2=(eye(size(H2))-H2);    

    rel_residual=norm(rhs1-B1*sol,2)/norm(rhs1,2);
    count=1;

    if stat.adapt_walks==1 || stat.adapt_cutoff==1
        while(rel_residual>eps && count<=rich_it)
           if mod(count,2)==1
               sol=sol+r; 
               r=rhs1-B1*sol;
               [dx, dvar, nwalks, rej]=MC_forward_adapt2(H1, r, P.P1, cdf.cdf1, stat);
               NWALKS=[NWALKS nwalks];
               reject{count}=rej;
               sol=sol+dx;
               r=rhs1-B1*sol;
               VAR=[VAR dvar];
               DX=[DX dx];
               rel_residual=norm(r,2)/norm(rhs1,2);
               REL_RES=[REL_RES rel_residual];  
           else
               sol=sol+r;
               r=rhs2-B2*sol;
               [dx, dvar, nwalks]=MC_forward_adapt2(H2, r, P.P2, cdf.cdf2, stat);
               NWALKS=[NWALKS nwalks];
               reject{count}=rej;
               sol=sol+dx;  
               r=rhs2-B2*sol;
               VAR=[VAR dvar];
               DX=[DX dx];
               rel_residual=norm(r,2)/norm(rhs2,2);
           end

           count=count+1;
        end
    else
       while(rel_residual>eps && count<=rich_it)
           X=[];
           if mod(count,2)==1
               sol=sol+r; 
               r=rhs1-B1*sol;
               [dx, dvar, X]=MC_forward(H1, r, P.P1, cdf.cdf1, n_walks, max_step);
               NWALKS=[NWALKS size(X,1)];
               sol=sol+dx;
               r=rhs1-B1*sol;
               VAR=[VAR dvar];
               DX=[DX dx];
               rel_residual=norm(r,2)/norm(rhs1,2);
               REL_RES=[REL_RES rel_residual];  
           else
               sol=sol+r;
               r=rhs2-B2*sol;
               [dx, dvar, X]=MC_forward(H2, r, P.P2, cdf.cdf2, n_walks, max_step);
               NWALKS=[NWALKS size(X,1)];
               sol=sol+dx;  
               r=rhs2-B2*sol;
               VAR=[VAR dvar];
               DX=[DX dx];
               rel_residual=norm(r,2)/norm(rhs2,2);
               REL_RES=[REL_RES rel_residual];  
           end
           count=count+1;
       end

        count=count-1;
        rel_res=rel_residual;

    end

end

tally=NWALKS;

end

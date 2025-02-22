function [sol, rel_res, VAR, REL_RES, DX, NWALKS, tally, count]=MCSA_adjoint1(fp, dist, P, cdf, numer, stat)

VAR=[];
rich_it=numer.rich_it;
DX=[];
NWALKS=[];
tally=[];
REL_RES=[];
n_walks=stat.nwalks;
max_step=stat.max_step;
eps=numer.eps;
rich_it=numer.rich_it;

if ~ strcmp(fp.precond, 'alternating')

%     if ~ asyn_check((fp.H)',P)
%         error('Iteration matrix does not have spectral radius less than 1');
%     end   

    display(strcat('Adjoint MCSA with', {' '}, fp.precond, ' preconditioning started'));

    H=fp.H;
    rhs=fp.rhs;
    sol=rand(size(H,1),1);

    %matrix to be used for the computation of the residual at each Richardson
    %iteration
    B=sparse((speye(size(H))-H));     

    rel_residual=norm(rhs-B*sol,2)/norm(rhs,2);
    count=1;

    r=rhs-B*sol;
    R=norm(r,2)/norm(rhs,2);
    REL_RES=[REL_RES R];
    var_ref=stat.varcut;
    
    if stat.adapt_walks==1 || stat.adapt_cutoff==1
        while(rel_residual>eps && count<=rich_it)
             display(strcat('iteration number', {' '}, num2str(count)))
             sol=sol+r; 
             r=rhs-B*sol;
             [Pb, cdfb]=prob_adjoint_rhs(r, dist);
             
             if stat.adapt_walks==1 && count>1 && REL_RES(end)>REL_RES(end-1)
                 stat.varcut=stat.varcut/ceil(REL_RES(end)/min(REL_RES));
             end
                     
             [dx, dvar, tot_walks, aux, resid]=MC_adjoint_adapt(H, r, P, cdf, Pb, cdfb, stat);
                         
             NWALKS=[NWALKS tot_walks];
             tally=[tally aux];
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
             [Pb, cdfb]=prob_adjoint_rhs(r, dist);
             [dx, dvar, tot_walks, aux]=MC_adjoint(H, r, P, cdf, Pb, cdfb, n_walks, max_step);
             NWALKS=[NWALKS tot_walks];
             tally=[tally aux];
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

    display('Adjoint MCSA with alternating method started');

    H1=fp.H1;
    H2=fp.H2;
    rhs1=fp.rhs1;
    rhs2=fp.rhs2;
    sol=ones(size(H1,1),1);

    %matrix to be used for the computation of the redisual at each Richardson
    %iteration
    B1=sparse(speye(size(H1))-H1);
    B2=sparse(speye(size(H2))-H2);

    rel_residual=norm(rhs1-B1*sol,2)/norm(rhs1,2);
    REL_RES=[REL_RES rel_residual];  
    count=1;

    VAR1=[];
    VAR2=[];


    if stat.adapt_walks==1 || stat.adapt_cutoff==1
        while(rel_residual>eps && count<=rich_it)
            if mod(count,2)==1
                sol=sol+r; 
                r=rhs1-B1*sol;
                [Pb, cdfb]=prob_adjoint_rhs(r, dist);

                if stat.adapt_walks==1 && count>1 && REL_RES(end)>REL_RES(end-1)
                   stat.varcut=stat.varcut/ceil(REL_RES(end)/min(REL_RES));
                end              
                
                [dx, dvar, tot_walks, aux, resid]=MC_adjoint_adapt(H1, r, P.P1, cdf.cdf1, Pb, cdfb, stat);
                NWALKS=[NWALKS tot_walks];
                tally=[tally aux];
                sol=sol+dx;
                r=rhs1-B1*sol;
                VAR1=[VAR1 dvar];
                VAR=[VAR dvar];
                DX=[DX dx];
                rel_residual=norm(r,2)/norm(rhs1,2);
                REL_RES=[REL_RES rel_residual];  
            else
                sol=sol+r; 
                r=rhs2-B2*sol;
                [Pb, cdfb]=prob_adjoint_rhs(r, dist);
                
                if count>1 && R(end)>R(end-1)
                   stat.varcut=stat.varcut/ceil(R(end)/min(R));
                end               
                
                [dx, dvar, tot_walks, aux]=MC_adjoint_adapt(H2, r, P.P2, cdf.cdf2, Pb, cdfb, stat);
                NWALKS=[NWALKS tot_walks];
                tally=[tally aux];
                sol=sol+dx;
                r=rhs2-B2*sol;
                VAR2=[VAR2 dvar];
                VAR=[VAR dvar];
                DX=[DX dx];
                rel_residual=norm(r,2)/norm(rhs2,2);
                REL_RES=[REL_RES rel_residual];  
            end
            count=count+1;
        end
        
    else
        while(rel_residual>eps && count<=rich_it)
            X=[];
            if mod(count,2)==1
                sol=sol+r; 
                r=rhs1-B1*sol;
                [Pb, cdfb]=prob_adjoint_rhs(r, dist);
                [dx, dvar, tot_walks, aux]=MC_adjoint(H1, r, P.P1, cdf.cdf1, Pb, cdfb, n_walks, max_step);
                NWALKS=[NWALKS tot_walks];
                tally=[tally aux];
                sol=sol+dx;
                r=rhs1-B1*sol;
                VAR1=[VAR1 dvar];
                VAR=[VAR dvar];
                DX=[DX dx];
                rel_residual=norm(r,2)/norm(rhs1,2);
                REL_RES=[REL_RES rel_residual];  
            else
                sol=sol+r; 
                r=rhs2-B2*sol;
                [Pb, cdfb]=prob_adjoint_rhs(r, dist);
                [dx, dvar, tot_walks, aux]=MC_adjoint(H2, r, P.P2, cdf.cdf2, Pb, cdfb, n_walks, max_step);
                NWALKS=[NWALKS tot_walks];
                tally=[tally aux];
                sol=sol+dx;
                r=rhs2-B2*sol;
                VAR2=[VAR2 dvar];
                VAR=[VAR dvar];
                DX=[DX dx];
                rel_residual=norm(r,2)/norm(rhs2,2);
                REL_RES=[REL_RES rel_residual];  
            end
            count=count+1;
        end

    end
               
    count=count-1;
    rel_res=rel_residual;


end

end



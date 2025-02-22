function [x, y, TALLY, time]=MC_adjoint_error(A, b, P, cdf, Pb, cdfb, n_walks, max_step)

    L=length(n_walks);
    
    x=zeros(size(A,1),L);
    y=zeros(size(A,1),L);
    time=[];
    TALLY=[];

    for i=1:L    
       X=zeros(n_walks(i),size(b,1));
       tally=zeros(size(b))';

       start=cputime;

       for walk=1:n_walks(i)
        aux=rand;

        %it detects what is the inital status of the chain
        previous=find(cdfb>aux, 1 );
        W=sum(abs(b))*sign(b(previous));
        tally(previous)=tally(previous)+1;
    %    W=b(previous)*length(find(P(previous,:)));
        X(walk,previous)=X(walk,previous)+W;
        step=1;
            while step<=max_step
                  aux=rand;
                  cdfrow_ind=find(cdf(previous,:));
                  if ~isempty(cdfrow_ind)
                    current=find(cdf(previous,cdfrow_ind)>aux, 1 );
                    current=cdfrow_ind(current);
                    W=W*A(current,previous)/P(previous,current);
                    % I insert a contribution on the estimator related to the
                    % current state
                    X(walk,current)=X(walk,current)+W;
                    step=step+1;
                    previous=current;                
                  else
                     W=0;
                  end
                  if W==0
                      % if I reach a null state, I will never move away from
                      % there and the related permutation will not give any
                      % contribution to the estimation of the updating vector,
                      % thus this permutation can be neglected
                     break;
                  end              
            end
       end

       finish=cputime;

        %computation of the expected value for the updating vector
       x(:,i)=mean(X,1)';
       Y=X.^2;
       y(:,i)=sqrt((mean(Y,1)'-(x(:,i).^2))./(size(X,1))); 
       time=[time finish-start];
       TALLY=[TALLY; tally];
    end
end




 
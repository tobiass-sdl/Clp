// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

/* 
   Authors
   
   John Forrest

 */
#ifndef ClpSimplex_H
#define ClpSimplex_H

#include <iostream>
#include <cfloat>
#include "ClpModel.hpp"
#include "ClpMatrixBase.hpp"
class ClpDualRowPivot;
class ClpPrimalColumnPivot;
class ClpFactorization;
class CoinIndexedVector;
class ClpNonLinearCost;

/** This solves LPs using the simplex method

    It inherits from ClpModel and all its arrays are created at
    algorithm time. Originally I tried to work with model arrays
    but for simplicity of coding I changed to single arrays with
    structural variables then row variables.  Some coding is still
    based on old style and needs cleaning up.

    For a description of algorithms:

    for dual see ClpSimplexDual.hpp and at top of ClpSimplexDual.cpp
    for primal see ClpSimplexPrimal.hpp and at top of ClpSimplexPrimal.cpp

    There is an algorithm data member.  + for primal variations
    and - for dual variations

    This file also includes (at end) a very simple class ClpSimplexProgress
    which is where anti-looping stuff should migrate to

*/

class ClpSimplex : public ClpModel {
   friend void ClpSimplexUnitTest(const std::string & mpsDir,
				  const std::string & netlibDir);

public:

  /// enums for status of various sorts (matches CoinWarmStartBasis)
  enum Status {
    isFree = 0x00,
    basic = 0x01,
    atUpperBound = 0x02,
    atLowerBound = 0x03,
    superBasic = 0x04
  };

  enum FakeBound {
    noFake = 0x00,
    bothFake = 0x01,
    upperFake = 0x02,
    lowerFake = 0x03
  };

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
    ClpSimplex (  );

  /// Copy constructor. 
  ClpSimplex(const ClpSimplex &);
  /// Copy constructor from model. 
  ClpSimplex(const ClpModel &);
  /// Assignment operator. This copies the data
    ClpSimplex & operator=(const ClpSimplex & rhs);
  /// Destructor
   ~ClpSimplex (  );
  // Ones below are just ClpModel with setti
  /** Loads a problem (the constraints on the
        rows are given by lower and upper bounds). If a pointer is 0 then the
        following values are the default:
        <ul>
          <li> <code>colub</code>: all columns have upper bound infinity
          <li> <code>collb</code>: all columns have lower bound 0 
          <li> <code>rowub</code>: all rows have upper bound infinity
          <li> <code>rowlb</code>: all rows have lower bound -infinity
	  <li> <code>obj</code>: all variables have 0 objective coefficient
        </ul>
    */
  void loadProblem (  const ClpMatrixBase& matrix,
		     const double* collb, const double* colub,   
		     const double* obj,
		     const double* rowlb, const double* rowub,
		      const double * rowObjective=NULL);
  void loadProblem (  const CoinPackedMatrix& matrix,
		     const double* collb, const double* colub,   
		     const double* obj,
		     const double* rowlb, const double* rowub,
		      const double * rowObjective=NULL);

  /** Just like the other loadProblem() method except that the matrix is
	given in a standard column major ordered format (without gaps). */
  void loadProblem (  const int numcols, const int numrows,
		     const CoinBigIndex* start, const int* index,
		     const double* value,
		     const double* collb, const double* colub,   
		     const double* obj,
		      const double* rowlb, const double* rowub,
		      const double * rowObjective=NULL);
  /// This one is for after presolve to save memory
  void loadProblem (  const int numcols, const int numrows,
		     const CoinBigIndex* start, const int* index,
		      const double* value,const int * length,
		     const double* collb, const double* colub,   
		     const double* obj,
		      const double* rowlb, const double* rowub,
		      const double * rowObjective=NULL);
  /// Read an mps file from the given filename
  int readMps(const char *filename,
	      bool keepNames=false,
	      bool ignoreErrors = false);
  /** Borrow model.  This is so we dont have to copy large amounts
      of data around.  It assumes a derived class wants to overwrite
      an empty model with a real one - while it does an algorithm.
      This is same as ClpModel one, but sets scaling on etc. */
  void borrowModel(ClpModel & otherModel);
  //@}

  /**@name Functions most useful to user */
  //@{
  /** Dual algorithm - see ClpSimplexDual.hpp for method */
  int dual();
  /** Primal algorithm - see ClpSimplexPrimal.hpp for method */
  int primal(int ifValuesPass=0);
  /// Passes in factorization
  void setFactorization( ClpFactorization & factorization);
  /// Sets or unsets scaling, 0 -off, 1 on, 2 dynamic(later)
  void scaling(int mode=1);
  /// Gets scalingFlag
  inline int scalingFlag() const {return scalingFlag_;};
  /** Tightens primal bounds to make dual faster.  Unless
      fixed, bounds are slightly looser than they could be.
      This is to make dual go faster and is probably not needed
      with a presolve.  Returns non-zero if problem infeasible
  */
  int tightenPrimalBounds();
  /// Sets row pivot choice algorithm in dual
  void setDualRowPivotAlgorithm(ClpDualRowPivot & choice);
  /// Sets column pivot choice algorithm in primal
  void setPrimalColumnPivotAlgorithm(ClpPrimalColumnPivot & choice);
  /** For strong branching.  On input lower and upper are new bounds
      while on output they are change in objective function values 
      (>1.0e50 infeasible).
      Return code is 0 if nothing interesting, -1 if infeasible both
      ways and +1 if infeasible one way (check values to see which one(s))
  */
  int strongBranching(int numberVariables,const int * variables,
		      double * newLower, double * newUpper,
		      bool stopOnFirstInfeasible=true,
		      bool alwaysFinish=false);
  //@}

  /**@name most useful gets and sets */
  //@{ 
  /// If problem is primal feasible
  inline bool primalFeasible() const
         { return (numberPrimalInfeasibilities_==0);};
  /// If problem is dual feasible
  inline bool dualFeasible() const
         { return (numberDualInfeasibilities_==0);};
  /// factorization 
  inline ClpFactorization * factorization() const 
          { return factorization_;};
  /// Sparsity on or off
  bool sparseFactorization() const;
  void setSparseFactorization(bool value);
  /// Dual bound
  inline double dualBound() const
          { return dualBound_;};
  void setDualBound(double value);
  /// Infeasibility cost
  inline double infeasibilityCost() const
          { return infeasibilityCost_;};
  void setInfeasibilityCost(double value);
  /** Amount of print out:
      0 - none
      1 - just final
      2 - just factorizations
      3 - as 2 plus a bit more
      4 - verbose
      above that 8,16,32 etc just for selective debug
  */
  /** Perturbation:
      -50 to +50 - perturb by this power of ten (-6 sounds good)
      100 - auto perturb if takes too long (1.0e-6 largest nonzero)
      101 - we are perturbed
      102 - don't try perturbing again
      default is 100
  */
  inline int perturbation() const
    { return perturbation_;};
  void setPerturbation(int value);
  /// Current (or last) algorithm
  inline int algorithm() const 
  {return algorithm_; } ;
  /// Sum of dual infeasibilities
  inline double sumDualInfeasibilities() const 
          { return sumDualInfeasibilities_;} ;
  /// Number of dual infeasibilities
  inline int numberDualInfeasibilities() const 
          { return numberDualInfeasibilities_;} ;
  /// Sum of primal infeasibilities
  inline double sumPrimalInfeasibilities() const 
          { return sumPrimalInfeasibilities_;} ;
  /// Number of primal infeasibilities
  inline int numberPrimalInfeasibilities() const 
          { return numberPrimalInfeasibilities_;} ;
  /** Save model to file, returns 0 if success.  This is designed for
      use outside algorithms so does not save iterating arrays etc.
  It does not save any messaging information. 
  Does not save scaling values.
  It does not know about all types of virtual functions.
  */
  int saveModel(const char * fileName);
  /** Restore model from file, returns 0 if success,
      deletes current model */
  int restoreModel(const char * fileName);
  
  /** Just check solution (for external use) - sets sum of
      infeasibilities etc */
  void checkSolution();
  //@}

  /******************** End of most useful part **************/
  /**@name Functions less likely to be useful to casual user */
  //@{
  /** Given an existing factorization computes and checks 
      primal and dual solutions.  Uses input arrays for variables at
      bounds.  Returns feasibility states */
  int getSolution (  const double * rowActivities,
		     const double * columnActivities);
  /** Given an existing factorization computes and checks 
      primal and dual solutions.  Uses current problem arrays for
      bounds.  Returns feasibility states */
  int getSolution ();
  /** Factorizes using current basis.  
      solveType - 1 iterating, 0 initial, -1 external 
      If 10 added then in primal values pass
      Return codes are as from ClpFactorization unless initial factorization
      when total number of singularities is returned
  */
  int internalFactorize(int solveType);
  /// Factorizes using current basis. For external use
  int factorize();
  /// Computes duals from scratch
  void computeDuals();
  /// Computes primals from scratch
  void computePrimals (  const double * rowActivities,
		     const double * columnActivities);
  /**
     Unpacks one column of the matrix into indexed array 
     Uses sequenceIn_
     Also applies scaling if needed
  */
  void unpack(CoinIndexedVector * rowArray);
  /**
     Unpacks one column of the matrix into indexed array 
     Slack if sequence>= numberColumns
     Also applies scaling if needed
  */
  void unpack(CoinIndexedVector * rowArray,int sequence);
  
  /** 
      This does basis housekeeping and does values for in/out variables.
      Can also decide to re-factorize
  */
  int housekeeping(double objectiveChange);
  /** This sets largest infeasibility and most infeasible and sum
      and number of infeasibilities (Primal) */
  void checkPrimalSolution(const double * rowActivities=NULL,
			   const double * columnActivies=NULL);
  /** This sets largest infeasibility and most infeasible and sum
      and number of infeasibilities (Dual) */
  void checkDualSolution();
  //@}
  /**@name Matrix times vector methods 
     They can be faster if scalar is +- 1
     These are covers so user need not worry about scaling
     Also for simplex I am not using basic/non-basic split */
  //@{
    /** Return <code>y + A * x * scalar</code> in <code>y</code>.
        @precond <code>x<code> must be of size <code>numColumns()</code>
        @precond <code>y<code> must be of size <code>numRows()</code> */
   void times(double scalar,
		       const double * x, double * y) const;
    /** Return <code>y + x * scalar * A</code> in <code>y</code>.
        @precond <code>x<code> must be of size <code>numRows()</code>
        @precond <code>y<code> must be of size <code>numColumns()</code> */
    void transposeTimes(double scalar,
				const double * x, double * y) const ;
  //@}

  /**@name most useful gets and sets */
  //@{ 
  /// Worst column primal infeasibility
  inline double columnPrimalInfeasibility() const 
          { return columnPrimalInfeasibility_;} ;
  /// Sequence of worst (-1 if feasible)
  inline int columnPrimalSequence() const 
          { return columnPrimalSequence_;} ;
  /// Worst row primal infeasibility
  inline double rowPrimalInfeasibility() const 
          { return rowPrimalInfeasibility_;} ;
  /// Sequence of worst (-1 if feasible)
  inline int rowPrimalSequence() const 
          { return rowPrimalSequence_;} ;
  /** Worst column dual infeasibility (note - these may not be as meaningful
      if the problem is primal infeasible */
  inline double columnDualInfeasibility() const 
          { return columnDualInfeasibility_;} ;
  /// Sequence of worst (-1 if feasible)
  inline int columnDualSequence() const 
          { return columnDualSequence_;} ;
  /// Worst row dual infeasibility
  inline double rowDualInfeasibility() const 
          { return rowDualInfeasibility_;} ;
  /// Sequence of worst (-1 if feasible)
  inline int rowDualSequence() const 
          { return rowDualSequence_;} ;
  /// Primal tolerance needed to make dual feasible (<largeTolerance)
  inline double primalToleranceToGetOptimal() const 
          { return primalToleranceToGetOptimal_;} ;
  /// Remaining largest dual infeasibility
  inline double remainingDualInfeasibility() const 
          { return remainingDualInfeasibility_;} ;
  /// Large bound value (for complementarity etc)
  inline double largeValue() const 
          { return largeValue_;} ;
  void setLargeValue( double value) ;
  /// Largest error on Ax-b
  inline double largestPrimalError() const
          { return largestPrimalError_;} ;
  /// Largest error on basic duals
  inline double largestDualError() const
          { return largestDualError_;} ;
  /// Largest difference between input primal solution and computed
  inline double largestSolutionError() const
          { return largestSolutionError_;} ;
  /// Basic variables pivoting on which rows
  inline const int * pivotVariable() const
          { return pivotVariable_;};
  /// Current dual tolerance
  inline double currentDualTolerance() const 
          { return dualTolerance_;} ;
  inline void setCurrentDualTolerance(double value)
          { dualTolerance_ = value;} ;
  /// Current primal tolerance
  inline double currentPrimalTolerance() const 
          { return primalTolerance_;} ;
  inline void setCurrentPrimalTolerance(double value)
          { primalTolerance_ = value;} ;
  /// How many iterative refinements to do
  inline int numberRefinements() const 
          { return numberRefinements_;} ;
  void setnumberRefinements( int value) ;
  /// Alpha (pivot element) for use by classes e.g. steepestedge
  inline double alpha() const { return alpha_;};
  /// Reduced cost of last incoming for use by classes e.g. steepestedge
  inline double dualIn() const { return dualIn_;};
  /// Pivot Row for use by classes e.g. steepestedge
  inline int pivotRow() const{ return pivotRow_;};
  /// value of incoming variable (in Dual)
  double valueIncomingDual() const;
  //@}

  protected:
  /**@name protected methods */
  //@{
  /// May change basis and then returns number changed
  int gutsOfSolution ( const double * rowActivities,
		       const double * columnActivities,
		       bool valuesPass=false);
  /// Does most of deletion (0 = all, 1 = most, 2 most + factorization)
  void gutsOfDelete(int type);
  /// Does most of copying
  void gutsOfCopy(const ClpSimplex & rhs);
  /** puts in format I like (rowLower,rowUpper) also see StandardMatrix 
      1 bit does rows, 2 bit does column bounds, 4 bit does objective(s).
      8 bit does solution scaling in
      16 bit does rowArray and columnArray indexed vectors
      and makes row copy if wanted, also sets columnStart_ etc
      Also creates scaling arrays if needed.  It does scaling if needed.
      16 also moves solutions etc in to work arrays
      On 16 returns false if problem "bad" i.e. matrix or bounds bad
  */
  bool createRim(int what,bool makeRowCopy=false);
  /** releases above arrays and does solution scaling out.  May also 
      get rid of factorization data */
  void deleteRim(bool getRidOfFactorizationData=true);
  /// Sanity check on input rim data (after scaling) - returns true if okay
  bool sanityCheck();
  //@}
  public:
  /**@name public methods */
  //@{
  /** Return row or column sections - not as much needed as it 
      once was.  These just map into single arrays */
  inline double * solutionRegion(int section)
  { if (!section) return rowActivityWork_; else return columnActivityWork_;};
  inline double * djRegion(int section)
  { if (!section) return rowReducedCost_; else return reducedCostWork_;};
  inline double * lowerRegion(int section)
  { if (!section) return rowLowerWork_; else return columnLowerWork_;};
  inline double * upperRegion(int section)
  { if (!section) return rowUpperWork_; else return columnUpperWork_;};
  inline double * costRegion(int section)
  { if (!section) return rowObjectiveWork_; else return objectiveWork_;};
  /// Return region as single array
  inline double * solutionRegion()
  { return solution_;};
  inline double * djRegion()
  { return dj_;};
  inline double * lowerRegion()
  { return lower_;};
  inline double * upperRegion()
  { return upper_;};
  inline double * costRegion()
  { return cost_;};
  inline Status getStatus(int sequence) const
  {return static_cast<Status> (status_[sequence]&7);};
  inline void setStatus(int sequence, Status status)
  {
    unsigned char & st_byte = status_[sequence];
    st_byte &= ~7;
    st_byte |= status;
  };
  /** Return sequence In or Out */
  inline int sequenceIn() const
  {return sequenceIn_;};
  inline int sequenceOut() const
  {return sequenceOut_;};
  /** Set sequenceIn or Out */
  inline void  setSequenceIn(int sequence)
  { sequenceIn_=sequence;};
  inline void  setSequenceOut(int sequence)
  { sequenceOut_=sequence;};
  /// Returns 1 if sequence indicates column
  inline int isColumn(int sequence) const
  { return sequence<numberColumns_ ? 1 : 0;};
  /// Returns sequence number within section
  inline int sequenceWithin(int sequence) const
  { return sequence<numberColumns_ ? sequence : sequence-numberColumns_;};
  /// Return row or column values
  inline double solution(int sequence)
  { return solution_[sequence];};
  /// Return address of row or column values
  inline double & solutionAddress(int sequence)
  { return solution_[sequence];};
  inline double reducedCost(int sequence)
   { return dj_[sequence];};
  inline double & reducedCostAddress(int sequence)
   { return dj_[sequence];};
  inline double lower(int sequence)
  { return lower_[sequence];};
  /// Return address of row or column lower bound
  inline double & lowerAddress(int sequence)
  { return lower_[sequence];};
  inline double upper(int sequence)
  { return upper_[sequence];};
  /// Return address of row or column upper bound
  inline double & upperAddress(int sequence)
  { return upper_[sequence];};
  inline double cost(int sequence)
  { return cost_[sequence];};
  /// Return address of row or column cost
  inline double & costAddress(int sequence)
  { return cost_[sequence];};
  /// Scaling
  const double * rowScale() const {return rowScale_;};
  const double * columnScale() const {return columnScale_;};
  void setRowScale(double * scale) { rowScale_ = scale;};
  void setColumnScale(double * scale) { columnScale_ = scale;};
  //@}
  /**@name status methods */
  //@{
  inline void setFakeBound(int sequence, FakeBound fakeBound)
  {
    unsigned char & st_byte = status_[sequence];
    st_byte &= ~24;
    st_byte |= fakeBound<<3;
  };
  inline FakeBound getFakeBound(int sequence) const
  {return static_cast<FakeBound> ((status_[sequence]>>3)&3);};
  inline void setRowStatus(int sequence, Status status)
  {
    unsigned char & st_byte = status_[sequence+numberColumns_];
    st_byte &= ~7;
    st_byte |= status;
  };
  inline Status getRowStatus(int sequence) const
  {return static_cast<Status> (status_[sequence+numberColumns_]&7);};
  inline void setColumnStatus(int sequence, Status status)
  {
    unsigned char & st_byte = status_[sequence];
    st_byte &= ~7;
    st_byte |= status;
  };
  inline Status getColumnStatus(int sequence) const
  {return static_cast<Status> (status_[sequence]&7);};
  inline void setFixed( int sequence)
  { status_[sequence] |= 32;};
  inline void clearFixed( int sequence)
  { status_[sequence] &= ~32; };
  inline bool fixed(int sequence) const
  {return (((status_[sequence]>>5)&1)!=0);};
  inline void setFlagged( int sequence)
  {
    status_[sequence] |= 64;
  };
  inline void clearFlagged( int sequence)
  {
    status_[sequence] &= ~64;
  };
  inline bool flagged(int sequence) const
  {return (((status_[sequence]>>6)&1)!=0);};
  /** Set up status array (can be used by OsiClp).
      Also can be used to set up all slack basis */
  void createStatus() ;
    
  /// So we know when to be cautious
  inline int lastBadIteration() const
  {return lastBadIteration_;};
  /// Progress flag - at present 0 bit says artificials out
  inline int progressFlag() const
  {return progressFlag_;};
  //@}

////////////////// data //////////////////
protected:

  /**@name data.  Many arrays have a row part and a column part.
   There is a single array with both - columns then rows and
   then normally two arrays pointing to rows and columns.  The
   single array is the owner of memory 
  */
  //@{
  /// Worst column primal infeasibility
  double columnPrimalInfeasibility_;
  /// Sequence of worst (-1 if feasible)
  int columnPrimalSequence_;
  /// Worst row primal infeasibility
  double rowPrimalInfeasibility_;
  /// Sequence of worst (-1 if feasible)
  int rowPrimalSequence_;
  /// Worst column dual infeasibility
  double columnDualInfeasibility_;
  /// Sequence of worst (-1 if feasible)
  int columnDualSequence_;
  /// Worst row dual infeasibility
  double rowDualInfeasibility_;
  /// Sequence of worst (-1 if feasible)
  int rowDualSequence_;
  /// Primal tolerance needed to make dual feasible (<largeTolerance)
  double primalToleranceToGetOptimal_;
  /// Remaining largest dual infeasibility
  double remainingDualInfeasibility_;
  /// Large bound value (for complementarity etc)
  double largeValue_;
  /// Largest error on Ax-b
  double largestPrimalError_;
  /// Largest error on basic duals
  double largestDualError_;
  /// Largest difference between input primal solution and computed
  double largestSolutionError_;
  /// Dual bound
  double dualBound_;
  /// Working copy of lower bounds (Owner of arrays below)
  double * lower_;
  /// Row lower bounds - working copy
  double * rowLowerWork_;
  /// Column lower bounds - working copy
  double * columnLowerWork_;
  /// Working copy of upper bounds (Owner of arrays below)
  double * upper_;
  /// Row upper bounds - working copy
  double * rowUpperWork_;
  /// Column upper bounds - working copy
  double * columnUpperWork_;
  /// Working copy of objective (Owner of arrays below)
  double * cost_;
  /// Row objective - working copy
  double * rowObjectiveWork_;
  /// Column objective - working copy
  double * objectiveWork_;
  /// Useful row length arrays 
  CoinIndexedVector * rowArray_[6];
  /// Useful column length arrays 
  CoinIndexedVector * columnArray_[6];
  /// Alpha (pivot element)
  double alpha_;
  /// Theta (pivot change)
  double theta_;
  /// Lower Bound on In variable
  double lowerIn_;
  /// Value of In variable
  double valueIn_;
  /// Upper Bound on In variable
  double upperIn_;
  /// Reduced cost of In variable
  double dualIn_;
  /// Sequence of In variable
  int sequenceIn_;
  /// Direction of In, 1 going up, -1 going down, 0 not a clude
  int directionIn_;
  /// Lower Bound on Out variable
  double lowerOut_;
  /// Value of Out variable
  double valueOut_;
  /// Upper Bound on Out variable
  double upperOut_;
  /// Infeasibility (dual) or ? (primal) of Out variable
  double dualOut_;
  /// Sequence of Out variable
  int sequenceOut_;
  /// Direction of Out, 1 to upper bound, -1 to lower bound, 0 - superbasic
  int directionOut_;
  /// Pivot Row
  int pivotRow_;
  /// Working copy of reduced costs (Owner of arrays below)
  double * dj_;
  /// Reduced costs of slacks not same as duals (or - duals)
  double * rowReducedCost_;
  /// Possible scaled reduced costs
  double * reducedCostWork_;
  /// Working copy of primal solution (Owner of arrays below)
  double * solution_;
  /// Row activities - working copy
  double * rowActivityWork_;
  /// Column activities - working copy
  double * columnActivityWork_;
  /// Current dual tolerance for algorithm
  double dualTolerance_;
  /// Current primal tolerance for algorithm
  double primalTolerance_;
  /// Sum of dual infeasibilities
  double sumDualInfeasibilities_;
  /// Number of dual infeasibilities
  int numberDualInfeasibilities_;
  /// Number of dual infeasibilities (without free)
  int numberDualInfeasibilitiesWithoutFree_;
  /// Sum of primal infeasibilities
  double sumPrimalInfeasibilities_;
  /// Number of primal infeasibilities
  int numberPrimalInfeasibilities_;
  /// dual row pivot choice
  ClpDualRowPivot * dualRowPivot_;
  /// primal column pivot choice
  ClpPrimalColumnPivot * primalColumnPivot_;
  /// Basic variables pivoting on which rows
  int * pivotVariable_;
  /// factorization 
  ClpFactorization * factorization_;
  /// How many iterative refinements to do
  int numberRefinements_;
  /// Row scale factors for matrix
  // ****** get working simply then make coding more efficient
  // on full matrix operations
  double * rowScale_;
  /// Saved version of solution
  double * savedSolution_;
  /// Column scale factors 
  double * columnScale_;
  /// Scale flag, 0 none, 1 normal, 2 dynamic
  int scalingFlag_;
  /// Number of times code has tentatively thought optimal
  int numberTimesOptimal_;
  /// If change has been made (first attempt at stopping looping)
  int changeMade_;
  /// Algorithm >0 == Primal, <0 == Dual
  int algorithm_;
  /** Now for some reliability aids
      This forces re-factorization early */
  int forceFactorization_;
  /// Saved status regions
  unsigned char * saveStatus_;
  /** Perturbation:
      -50 to +50 - perturb by this power of ten (-6 sounds good)
      100 - auto perturb if takes too long (1.0e-6 largest nonzero)
      101 - we are perturbed
      102 - don't try perturbing again
      default is 100
  */
  int perturbation_;
  /// Weight assigned to being infeasible in primal
  double infeasibilityCost_;
  /** Very wasteful way of dealing with infeasibilities in primal.
      However it will allow non-linearities and use of dual
      analysis.  If it doesn't work it can easily be replaced.
  */
  ClpNonLinearCost * nonLinearCost_;
  /// For advanced options
  unsigned int specialOptions_;
  /// So we know when to be cautious
  int lastBadIteration_;
  /// Can be used for count of fake bounds (dual) or fake costs (primal)
  int numberFake_;
  /// Progress flag - at present 0 bit says artificials out
  int progressFlag_;
  /// Sum of Dual infeasibilities using tolerance based on error in duals
  double sumOfRelaxedDualInfeasibilities_;
  /// Sum of Primal infeasibilities using tolerance based on error in primals
  double sumOfRelaxedPrimalInfeasibilities_;
  //@}
};
//#############################################################################
/** A function that tests the methods in the ClpSimplex class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging.

    It also does some testing of ClpFactorization class
 */
void
ClpSimplexUnitTest(const std::string & mpsDir,
		   const std::string & netlibDir);


/// For saving extra information to see if looping. not worth a Class
class ClpSimplexProgress {

public:


  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
    ClpSimplexProgress (  );

  /// Constructor from model
    ClpSimplexProgress ( ClpSimplex * model );

  /// Copy constructor. 
  ClpSimplexProgress(const ClpSimplexProgress &);

  /// Assignment operator. This copies the data
    ClpSimplexProgress & operator=(const ClpSimplexProgress & rhs);
  /// Destructor
   ~ClpSimplexProgress (  );
  //@}

  /**@name Check progress */
  //@{
  /** Returns -1 if okay, -n+1 (n number of times bad) if bad but action taken,
      >=0 if give up and use as problem status
  */
    int looping (  );

  //@}
  /**@name Data  */
#define CLP_PROGRESS 5
  //@{
  /// Objective values
  double objective_[CLP_PROGRESS];
  /// Sum of infeasibilities for algorithm
  double infeasibility_[CLP_PROGRESS];
  /// Number of infeasibilities
  int numberInfeasibilities_[CLP_PROGRESS];
  /// Number of times checked (so won't stop too early)
  int numberTimes_;
  /// Number of times it looked like loop
  int numberBadTimes_;
  /// Pointer back to model so we can get information
  ClpSimplex * model_;
  //@}
};
#endif
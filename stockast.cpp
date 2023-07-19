#include <iostream>
#include <cstring>
#include <chrono>
// include <omp.h>
#include <random>

#include "gnuplot-iostream.h"

using namespace std;

// Function Declarations
float calcVolatility(const float &i_spot, const int &i_timesteps);
float* find2DMean(float **i_matrix, const int &i_M, const int &i_N);
float randGen(const float &i_mean, const float &i_sd);
float* runBlackScholesModel(const float &i_sp, const int &i_n, const float &i_r, const float &i_sig);

// Main function
int main(int i_argc, char **i_argv) {
    int l_plotFlag;    // Flag indicating whether to plot or not

    // Check for input terminal arguments. If none specified, plot by default.
    if (i_argc != 2) {
        l_plotFlag = 1;
    } else {
        l_plotFlag = atoi(i_argv[1]);
    } // if-else


    // Start time
    clock_t l_t = clock();

    // Variable declaration
    int     l_i, l_j;       // Loop iterators
    int     l_inLoops;      // Inner loop iterations
    int     l_outLoops;     // Outer loop iterations
    int     l_timesteps;    // Stock market time-intervals (min)
    int     l_numThreads;   // Number of threads
    float   l_riskRate;     // Risk free interest rate (%)
    float   l_sigma;        // Market volatility (calculated from data.csv)
    float   l_spot;         // Spot price (at t = 0)
    float **l_stock;        // Matrix for stock-price vectors per iteration
    float **l_avgStock;     // Matrix for mean of stock-price vectors per iteration
    float  *l_optStock;     // Vector for most likely outcome stock price
    FILE   *l_fp;           // File pointer

    // Initialization
    l_timesteps = 180;
    l_inLoops   = 100;
    l_outLoops  = 10000;
    l_spot      = 100.0f;
    l_riskRate  = 0.001f;
    l_sigma     = calcVolatility(l_spot, l_timesteps);

  // Memory allocation
    l_stock = new float *[l_inLoops];
    for (l_i = 0; l_i < l_inLoops; l_i++) {
        l_stock[l_i] = new float [l_timesteps];
    } // for

    l_avgStock  = new float *[l_outLoops];
    for( l_i = 0; l_i < l_outLoops; l_i++ )
    l_avgStock[l_i] = new float [l_timesteps];

    l_optStock  = new float [l_timesteps];

    // Welcome message
    cout << "--Welcome to Stockast: Stock Forecasting Tool--\n";
    cout << "  Parth Shah, Premanand Kumar, Rajdeep Konwar  \n\n";
    cout << "  Using market volatility = " << l_sigma << endl;

  // Parallel region with each thread having its own instance of variable 'i',
#pragma omp parallel private(l_i)
    {
    // Only one thread (irrespective of thread id) handles this region
//#pragma omp single
//    {
//      l_numThreads  = omp_get_num_threads();
//      cout << "  Using " << l_numThreads << " thread(s)..\n";
//      omp_set_num_threads(l_numThreads);
//    }

    /**
     Parallel for loop with dynamic scheduling, i.e. each thread
     grabs "chunk" iterations until all iterations are done.
     Faster threads are assigned more iterations (not Round Robin)
     */
        
    #pragma omp for schedule(dynamic)
        for (l_i = 0; l_i < l_outLoops; l_i++) {
          /**
           Using Black Scholes model to get stock price every iteration
           Returns data as a column vector having rows=timesteps
           */
          for (l_j = 0; l_j < l_inLoops; l_j++)
            l_stock[l_j]  = runBlackScholesModel(l_spot, l_timesteps, l_riskRate,
                                                  l_sigma);

          // Stores average of all estimated stock-price arrays
          l_avgStock[l_i] = find2DMean(l_stock, l_inLoops, l_timesteps);
        } // for
        
    // --> Implicit omp barrier <--
    } // omp parallel private(l_i)

    // Average of all the average arrays
    l_optStock  = find2DMean(l_avgStock, l_outLoops, l_timesteps);

    // Write optimal outcome to disk
    l_fp  = fopen("opt.csv", "w");
    if (!l_fp) {
        cerr << "Couldn't open opt.csv! Exiting..\n";
        return EXIT_FAILURE;
    } // if

    
    for (l_i = 0; l_i < l_timesteps; l_i++) {
        fprintf(l_fp, "%f\n", l_optStock[l_i]);
    } // for
        
    // Close the file pointer
    fclose(l_fp);

  // Memory deallocation
    for (l_i = 0; l_i < l_inLoops; l_i++) {
        delete[] l_stock[l_i];
    } // for
    delete[] l_stock;

    for (l_i = 0; l_i < l_outLoops; l_i++) {
        delete[] l_avgStock[l_i];
    }
    delete[] l_avgStock;

    delete[] l_optStock;

//    // Plot the most likely outcome in Gnuplot if plotFlag = 1
//    if (l_plotFlag) {
//        Gnuplot l_gp( "gnuplot -persist");
//        l_gp << "set grid\n";
//        l_gp << "set title 'Stock Market Forecast (3 hrs)'\n";
//        l_gp << "set xlabel 'Time (min)'\n";
//        l_gp << "set ylabel 'Price'\n";
//        l_gp << "plot 'opt.csv' w l title 'Predicted Stock Price'\n";
//    } // if

    // Finish time
     l_t = clock() - l_t;
     cout << "  Time taken = " << (float) l_t / CLOCKS_PER_SEC << "s\n\n";

      return EXIT_SUCCESS;
} // main()



// Simulates Black Scholes model
float * runBlackScholesModel( const float &i_sp,
                              const int   &i_n,
                              const float &i_r,
                              const float &i_sig ) {
  int    l_i;                           // Loop counter
  float  l_mean   = 0.0f, l_sd = 1.0f;  /  Mean and standard deviation
  float  l_deltaT = 1.0f / i_n;         //  Timestep
  float *l_z      = new float [i_n-1];  //  Array of normally distributed random nos.
  float *l_st     = new float [i_n];    //  Array of stock price at diff. times
  l_st[0]         = i_sp;               //  Stock price at t=0 is spot price

  // Populate array with random nos.
  for( l_i = 0; l_i < i_n - 1; l_i++ )
    l_z[l_i]  = randGen( l_mean, l_sd );

  //  Apply Black Scholes equation to calculate stock price at next timestep
  for( l_i = 0; l_i < i_n - 1; l_i++ )
    l_st[l_i+1] = l_st[l_i] * exp( ((i_r - (powf( i_sig, 2.0f ) / 2.0f)) * l_deltaT)
                                 + (i_sig * l_z[l_i] * sqrtf( l_deltaT )) );

  delete[] l_z;

  return l_st;
} // runBlackScholesModel()


/**
 Finds mean of a 2D array across first index (inLoops)
 M is in/outLoops and N is timesteps
 */
float * find2DMean(float **i_matrix,
                    const int    &i_M,
                    const int    &i_N ) {
  int    l_i, l_j;
  float *l_avg = new float [i_N];
  float  l_sum = 0.0f;

  for( l_i = 0; l_i < i_N; l_i++ ) {
    /**
     A private copy of 'sum' variable is created for each thread.
     At the end of the reduction, the reduction variable is applied to
     all private copies of the shared variable, and the final result
     is written to the global shared variable.
     */
#pragma omp parallel for private(l_j) reduction(+:l_sum)
    for( l_j = 0; l_j < i_M; l_j++ ) {
      l_sum += i_matrix[l_j][l_i];
    }

    //! Calculating average across columns
    l_avg[l_i] = l_sum / i_M;
    l_sum = 0.0f;
  }

  return l_avg;
}

/**
 Generates a random number seeded by system clock based on standard
 normal distribution on taking mean 0.0 and standard deviation 1.0
 */
float randGen( const float &i_mean,
               const float &i_sd ) {
  unsigned l_seed = chrono::system_clock::now().time_since_epoch().count();
  default_random_engine l_generator( l_seed );
  normal_distribution< float > l_distribution( i_mean, i_sd );

  return l_distribution( l_generator );
}

//  Calculates volatility from ml_data.csv file
float calcVolatility( const float &i_spot,
                      const int   &i_timesteps) {
  int   l_i, l_len = i_timesteps - 1;
  char  l_line[4096], *l_token;
  float l_mean, l_sum, l_sd, l_priceArr[l_len];
  FILE* l_fp;

  //  Open ml_data.csv in read-mode, exit on fail
  l_fp = std::fopen( "ml_data.csv", "r");
  if( !l_fp ) {
    std::cerr << "Cannot open ml_data.csv! Exiting..\n";
    exit( EXIT_FAILURE );
  }

  // Read the first line then close file
  if( fgets( l_line, sizeof( l_line ), l_fp ) == nullptr ) {
    std::cerr << "Cannot read from ml_data.csv! Exiting..\n";
    fclose( l_fp );
    exit( EXIT_FAILURE );
  }
  fclose( l_fp );

  // Get the return values of stock from file (min 2 to 180)
  for( l_i = 0; l_i < l_len; l_i++ ) {
    if( l_i == 0 )
      l_token = strtok( l_line, "," );
    else
      l_token   = strtok( NULL, "," );
    l_priceArr[l_i] = std::atof( l_token );
  }

  l_sum = i_spot;
  // Find mean of the estimated minute-end prices
  for( l_i = 0; l_i < l_len; l_i++ )
    l_sum += l_priceArr[l_i];
  l_mean = l_sum / (l_len + 1);

  //!Calculate market volatility as standard deviation
  l_sum = powf( (i_spot - l_mean), 2.0f );
  for( l_i = 0; l_i < l_len; l_i++ )
    l_sum += powf( (l_priceArr[l_i] - l_mean), 2.0f );
  l_sd = sqrtf( l_sum );

  // Return as percentage
  return (l_sd / 100.0f);
}

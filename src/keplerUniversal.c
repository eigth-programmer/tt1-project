#include "keplerUniversal.h"
#include "algebraFunctions.h"
#include <stdlib.h>
#include <math.h>
#ifdef INFINITY
/* INFINITY is supported */
#endif
/**
* Most effecient way to propagate any type of two body orbit using kepler's equations.
*
* @param rows matrixs column's number
* @param columns matrixs column's number
* @param r0 [3 x n] matrix, position Vector in ECI coordinate frame of reference
* @param v0 [3 x n] matrix, velocity Vector in ECI coordinate frame of reference
* @param timeVector [1 x n] matrix, time vector in seconds
* @param mu Gravitional Constant, if not specified value is Earth's mu constant
*
*/
void keplerUniversal(int rows, int columns, double **r0, double **v0, double *timeVector, double mu, double ***rA, double ***vA){
    
    double tolerance = 1e-9;
    
    double **v0Pow2;
    double v0Compressed[columns];
    double v0Mag[columns];

    matrixPow(rows, columns, 2, v0, &v0Pow2);
    sumMatrixRows(rows, columns, v0, v0Compressed);

    double **r0Pow2;
    double r0Compressed[columns];
    double r0Mag[columns];

    matrixPow(rows, columns, 2, r0, &r0Pow2);
    sumMatrixRows(rows, columns, r0, r0Compressed);

    double alpha[columns];

    for(int i=0; i < columns; i++){
        r0Mag[i] = sqrt(r0Compressed[i]);
        v0Mag[i] = sqrt(v0Compressed[i]);
        alpha[i] = -pow(v0Mag[i],2)/mu + 2/r0Mag[i];
    }

    freeMatrix(rows, v0Pow2);
    freeMatrix(rows, r0Pow2);

    double x0[columns];

    int idx[columns];
    elemGreaterThanValue(columns, 0.000001, alpha, idx);
    
    // Check if there are any Eliptic/Circular orbits
    if(any(columns, idx) == 1){
        for(int i = 0; i < columns; i++){
            if(idx[i] == 1){
                x0[i] = sqrt(mu)*timeVector[i]*alpha[i];
            }
        }
    }
    
    double absAlpha[columns];
    elemLowerThanValue(columns, 0.000001, absAlpha, idx);
    //Check if there are any Parabolic orbits

    if(any(columns, idx) == 1){
        double **h = (double **) calloc(rows, sizeof(double *));

        int col = truesInArray(columns, idx);
        double r0idx[rows][col];
        double v0idx[rows][col];
        double r0column[rows]; 
        double v0column[rows];

        for(int i = 0; i < columns; i++){
            getColumn(rows, i, r0, r0column);
            getColumn(rows, i, v0, v0column);
            for(int j = 0; j < rows; j++){
                r0idx[j][i] = r0column[j];
                v0idx[j][i] = v0column[j];
            }
        }  
 
        double crossColumn[rows];

        for(int i = 0; i < columns; i++){
            getColumn(rows, i, r0, r0column);
            getColumn(rows, i, v0, v0column);
            crossProduct(rows, r0column, v0column, crossColumn);
            for(int j = 0; j < rows; j++){
                h[j][i] = r0column[j];
            }
        } 
        
        double **hPow2; 
        double hCompressed[columns];
        double hMag[columns];

        matrixPow(rows, columns, 2, h, &hPow2);
        sumMatrixRows(rows, columns, hPow2, hCompressed);

        double p[columns];
        double s[columns];
        double w[columns];

        for(int i = 0; i < columns; i++){
            hMag[i] = sqrt(hCompressed[i]); 
            p[i] = pow(hMag[i],2)/mu;
            if(idx[i] == 1) s[i] = (1/(1/tan(3*sqrt(mu/pow(p[i], 3)))))*timeVector[i]/2;
            w[i] = atan(pow(tan(s[i]), 1/3));
            if(idx[i] == 1) x0[i] = sqrt(p[i])*2*(1/tan(2*w[i]));
        }

        freeMatrix(rows, h);
    }

    //Check if there are any Hyperbolic orbits
    elemLowerThanValue(columns, -0.000001, alpha, idx);
    if(any(columns, idx) == 1){

        int col = truesInArray(columns, idx);
        double **r0idx = (double **) calloc(rows, sizeof(double *)); 
        double **v0idx = (double **) calloc(rows, sizeof(double *));
        double r0column[rows];
        double v0column[rows];

        for(int i = 0; i < columns; i++){
            getColumn(rows, i, r0, r0column);
            getColumn(rows, i, v0, v0column);
            for(int j = 0; j < rows; j++){
                r0idx[j][i] = r0column[j];
                v0idx[j][i] = v0column[j];
            }
        } 

        double dot = dotProduct(rows, col, r0idx, v0idx); 

        for(int i =0; i < columns; i++){
            if(idx[i] == 1){
                double a = 1/alpha[i];
                x0[i] = sign(timeVector[i])*sqrt(-a)*log(-2*mu*alpha[i]*timeVector[i]/(dot + sign(timeVector[i])*sqrt(-mu*a)*(1 - r0Mag[i]*alpha[i])));
            }
        }
    }


    double error[columns];
    double dr0v0Smu = dotProduct(rows, columns, r0, v0)/sqrt(mu);
    double Smut[columns];
    multiplyArrayByScalar(columns, timeVector, sqrt(mu), Smut);

    double x02[columns]; 
    double x03[columns];
    double c2[columns]; 
    double c3[columns];
    double psi[columns];
    double r[columns];
    double xn[columns];
    double X0tOmPsiC3[columns];
    double X02tC2[columns];

    int a[columns];
    elemGreaterThanValue(columns, tolerance, error, a);
    while(any(columns, a) == 1){

        arrayPow(columns, 2, x0, x02);
        multiplyArrays(columns, x0, x02, x03);
        multiplyArrays(columns, x02, alpha, psi);

        c2c3(columns, psi, c2, c3);
        
        for(int i = 0; i < columns; i++){
            X0tOmPsiC3[i] = x0[i]*(1 - psi[i]*c3[i]);
            X02tC2[i] = x02[i]*c2[i];
            r[i] = X02tC2[i] + dr0v0Smu*X0tOmPsiC3[i] + r0Mag[i]*(1 - psi[i]*c2[i]);
            xn[i] = x0[i] + (Smut[i] - x03[i]*c3[i] - dr0v0Smu*X02tC2[i] - r0Mag[i]*X0tOmPsiC3[i])/r[i];
            error[i] = xn[i] - x0[i];
            x0[i] = xn[i];
        }

        elemGreaterThanValue(columns, tolerance, error, a);
    }

    double f[columns];
    double g[columns];
    double gdot[columns];
    double fdot[columns];

    for(int i=0; i < columns; i++){
        f[i] = 1 - pow(xn[i], 2)*c2[i]/r0Mag[i];
        g[i] = timeVector[i] - pow(xn[i], 3)*c3[i]/sqrt(mu);
        gdot[i] = 1 - c2[i]*pow(xn[i], 2)/r[i];
        fdot[i] = xn[i]*(psi[i]*c3[i]-1)*sqrt(mu)/r[i]*r0Mag[i];
    }

    double **vFinal = (double **) calloc(rows, sizeof(double *));
    double **fr0 = (double **) calloc(rows, sizeof(double *));
    double **gv0 = (double **) calloc(rows, sizeof(double *));

    timesArrayMatrix(columns, columns, f, r0, &fr0);
    timesArrayMatrix(columns, columns, g, v0, &gv0);
    addMatrixs(columns, columns, fr0, gv0, &vFinal);

    double **rFinal = (double **) calloc(rows, sizeof(double *));
    double **fdotr0 = (double **) calloc(rows, sizeof(double *));
    double **gdotv0 = (double **) calloc(rows, sizeof(double *));

    timesArrayMatrix(columns, columns, fdot, r0, &fdotr0);
    timesArrayMatrix(columns, columns, gdot, v0, &gdotv0);
    addMatrixs(columns, columns, fdotr0, gdotv0, &rFinal);

    freeMatrix(rows, fr0);
    freeMatrix(rows, gv0);
    freeMatrix(rows, fdotr0);
    freeMatrix(rows, gdotv0);

    *rA = rFinal;
    *vA = vFinal;
}

/**
* Returns matrix 
*
* @param n (in)
* @param psi (in)
* @param c2 (out)
* @param c3 (out)
*/
void c2c3(int n, double *psi, double *c2, double *c3)
{
    int idx[n];

    elemGreaterThanValue(n, 1e-6, psi, idx);
    if(any(n, idx) == 1){
        for(int i = 0; i < n; i++){
            if(idx[i] == 1){
                c2[i] = (1-cos(sqrt(psi[i])))/psi[i];
                c3[i] = (sqrt(psi[i]) - sin(sqrt(psi[i])))/sqrt(pow(psi[i],3));
            }
        }
    }
    
    elemLowerThanValue(n, -1e-6, psi, idx);
    if(any(n, idx) == 1){
        for(int i = 0; i < n; i++){
            if(idx[i] == 1){
                c2[i] = (1 - cosh(sqrt(-psi[i])))/psi[i];
                c3[i] = (sinh(sqrt(-psi[i])) - sqrt(-psi[i]))/sqrt(pow(-psi[i],3));
            }
        }
    }

    double absPsi[n];
    absArray(n, psi, absPsi);
    elemLowerOrEqualThanValue(n, -1e-6, absPsi, idx);
    if(any(n, idx) == 1){
        for(int i = 0; i < n; i++){
            if(idx[i] == 1){
                c2[i] = 0.5;
                c3[i] = 1/6;
            }
        }
    }
}
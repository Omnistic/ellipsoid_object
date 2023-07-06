#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

int __declspec(dllexport) APIENTRY UserObjectDefinition(double *data, double *tri_list);
int __declspec(dllexport) APIENTRY UserParamNames(char *data);

#define PI 3.14159265358979323846

BOOL WINAPI DllMain (HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved) {
   return TRUE;
}

/* the data is stored as follows:

For calls to UserObjectDefinition:
data[1]  = code indicating what data the DLL should compute (see code sample below)
data[10] - data[12] = data back from the DLL (see code sample below)

data[100] = the number of data parameters passed to the DLL
data[101+] = the value of parameter 1, 2, etc...

For calls to UserParamNames:
data = the parameter number whose name is requested, stored as an ASCII integer;

*/

int __declspec(dllexport) APIENTRY UserObjectDefinition(double *data, double *tri_list) {
	int code, n_theta, n_phi, is_volume, is_reflective;
	double aa, bb, cc;

	/* Semi-axes lengths */
	aa = data[101];
	bb = data[102];
	cc = data[103];

	/* Sampling */
	n_theta = (int)data[104];
	n_phi = (int)data[105];

	/* Volume flag */
	if (data[106] > 0.0) {
		is_volume = 1;
	}
	else {
		is_volume = 0;
	}

	/* Reflective flag */
	if (data[107] > 0.0) {
		is_reflective = 10.0;
	}
	else {
		is_reflective = 0.0;
	}

	/* Basic error handling */
	if (aa <= 0.0) aa = 1.0;
	if (bb <= 0.0) bb = 1.0;
	if (cc <= 0.0) cc = 1.0;
	if (n_theta < 4) n_theta = 10;
	if (n_phi < 3) n_phi = 10;

	/* what we do now depends upon what was requested */
	code = (int)data[1];

	switch(code) {
		/* basic data */
		case 0:
			/* Compute the total number of triangular facets used to render and trace this object */
			/* we need N on each end cap, 2N along the face, and 2 on the flat back */
			/* put this value in data[10] */
			data[10] = (double) 2 * ( n_theta - 1 ) * n_phi;
			
			/* is this object a solid? put 1 in data[11], use 0 if shell */
			data[11] = is_volume;

			/* is this object potentially diffractive? put in data[12] the CSG number that is diffractive, cannot use 0. Use 0 if not diffractive */
			data[12] = 0;

			return 0;

		/* generate triangles */
		case 1:
			{
			/*
			We are being asked to generate the triangle list.
			A triangle consists of 3 triplets of coordinates (x, y, and z for each of the 3 corners),
			plus a 6 digit integer that indicates the properties of the triangle.
			That is 10 numbers for each triangle.
			The triangle data will be stored in the tri_list array

			The 6 digit number is constructed as follows.

			The "visible" flag is in the 1's place. If all 3 sides of a
			triangle are visible, use 0. If the side from point 1 to point
			2 is invisible, use 1. If the side from point 2 to point 3 is
			invisible, use 2. If the side from point 3 to point 1 is invisible,
			use 4. Add these flags if more than one side is invisible. For example,
			if all 3 sides are invisible, use 7. Note invisible simply means the
			side is not drawn on the 3D Layout plot, it has nothing to do with ray
			tracing.

			The "reflective" flag is in the 10's place, use zero for refractive and 1 for reflective.

			The "coat scatter group" is a 2 digit integer in the 1,000 and 100's place.
			Only CSG 0-3 are currently supported, but 2 digits are provided for future
			expansion.

			The "exact group" flag is a 2 digit integer in the 100,000 and 10,000's place.
			The exact group number indicates which set of exact formulas are used to iterate
			to the actual object surface. Use 0 to indicate the flat triangular facet is the
			correct solution. The exact flags are used in the intercept portion of the code that is case 2 below.

			Example: 030212 means the exact group is 3, the CSG is 2, the facet
			is reflective, and the side of the triangle from point 2 to point 3 is not drawn.

			*/

			int num_triangles;
			double x_const_0, x_const_1, y_const_0, y_const_1, phi_0, phi_1, z_val_0, z_val_1;

			num_triangles = 0;

			/* Coordinates of poles */
			double north_pole[3] = { 0.0, 0.0, cc };
			double south_pole[3] = { 0.0, 0.0, -cc };

			for (int jj = 1; jj < n_theta / 2 + 1; jj++) {
				x_const_0 = aa * sin( jj * PI / n_theta );
				y_const_0 = bb * sin( jj * PI / n_theta );
				z_val_0 = cc * cos( jj * PI / n_theta );

				if (jj == 1) {
					/* Create the caps with triangles */
					for (int ii = 0; ii < n_phi; ii++) {
						phi_0 = (double)ii / n_phi * 2 * PI;
						phi_1 = ((double)ii + 1) / n_phi * 2 * PI;

						tri_list[num_triangles * 10 + 0] = north_pole[0];   // x1
						tri_list[num_triangles * 10 + 1] = north_pole[1]; // y1
						tri_list[num_triangles * 10 + 2] = north_pole[2]; // z1
						tri_list[num_triangles * 10 + 3] = x_const_0 * cos(phi_0);   // x2
						tri_list[num_triangles * 10 + 4] = y_const_0 * sin(phi_0); // y2
						tri_list[num_triangles * 10 + 5] = z_val_0;   // z2
						tri_list[num_triangles * 10 + 6] = x_const_0 * cos(phi_1);  // x3
						tri_list[num_triangles * 10 + 7] = y_const_0 * sin(phi_1); // y3
						tri_list[num_triangles * 10 + 8] = z_val_0;   // z3
						tri_list[num_triangles * 10 + 9] = 010000.0 + is_reflective;   // exact 0, CSG 1, refractive, 3-1 invisible
						num_triangles++;

						/* Mirror */
						tri_list[num_triangles * 10 + 0] = south_pole[0];   // x1
						tri_list[num_triangles * 10 + 1] = south_pole[1]; // y1
						tri_list[num_triangles * 10 + 2] = south_pole[2]; // z1
						tri_list[num_triangles * 10 + 3] = x_const_0 * cos(phi_0);   // x2
						tri_list[num_triangles * 10 + 4] = y_const_0 * sin(phi_0); // y2
						tri_list[num_triangles * 10 + 5] = -z_val_0;   // z2
						tri_list[num_triangles * 10 + 6] = x_const_0 * cos(phi_1);  // x3
						tri_list[num_triangles * 10 + 7] = y_const_0 * sin(phi_1); // y3
						tri_list[num_triangles * 10 + 8] = -z_val_0;   // z3
						tri_list[num_triangles * 10 + 9] = 010000.0 + is_reflective;   // exact 0, CSG 1, refractive, 3-1 invisible
						num_triangles++;
					}
				}
				else
				{
					x_const_1 = aa * sin(((double)jj - 1) * PI / n_theta);
					y_const_1 = bb * sin(((double)jj - 1) * PI / n_theta);
					z_val_1 = cc * cos(((double)jj - 1) * PI / n_theta);

					for (int ii = 0; ii < n_phi; ii++)
					{
						phi_0 = (double)ii / n_phi * 2 * PI;
						phi_1 = ((double)ii + 1) / n_phi * 2 * PI;
						
						tri_list[num_triangles * 10 + 0] = x_const_1 * cos(phi_0);   // x1
						tri_list[num_triangles * 10 + 1] = y_const_1 * sin(phi_0); // y1
						tri_list[num_triangles * 10 + 2] = z_val_1; // z1
						tri_list[num_triangles * 10 + 3] = x_const_1 * cos(phi_1);   // x2
						tri_list[num_triangles * 10 + 4] = y_const_1 * sin(phi_1); // y2
						tri_list[num_triangles * 10 + 5] = z_val_1;   // z2
						tri_list[num_triangles * 10 + 6] = x_const_0 * cos(phi_0);  // x3
						tri_list[num_triangles * 10 + 7] = y_const_0 * sin(phi_0); // y3
						tri_list[num_triangles * 10 + 8] = z_val_0;   // z3
						tri_list[num_triangles * 10 + 9] = 010002.0 + is_reflective;   // exact 0, CSG 1, refractive, 3-1 invisible
						num_triangles++;

						tri_list[num_triangles * 10 + 0] = x_const_1 * cos(phi_1);   // x1
						tri_list[num_triangles * 10 + 1] = y_const_1 * sin(phi_1); // y1
						tri_list[num_triangles * 10 + 2] = z_val_1; // z1
						tri_list[num_triangles * 10 + 3] = x_const_0 * cos(phi_0);   // x2
						tri_list[num_triangles * 10 + 4] = y_const_0 * sin(phi_0); // y2
						tri_list[num_triangles * 10 + 5] = z_val_0;   // z2
						tri_list[num_triangles * 10 + 6] = x_const_0 * cos(phi_1);  // x3
						tri_list[num_triangles * 10 + 7] = y_const_0 * sin(phi_1); // y3
						tri_list[num_triangles * 10 + 8] = z_val_0;   // z3
						tri_list[num_triangles * 10 + 9] = 010001.0 + is_reflective;   // exact 0, CSG 1, refractive, 3-1 invisible
						num_triangles++;

						/* Mirror */
						tri_list[num_triangles * 10 + 0] = x_const_1 * cos(phi_0);   // x1
						tri_list[num_triangles * 10 + 1] = y_const_1 * sin(phi_0); // y1
						tri_list[num_triangles * 10 + 2] = -z_val_1; // z1
						tri_list[num_triangles * 10 + 3] = x_const_1 * cos(phi_1);   // x2
						tri_list[num_triangles * 10 + 4] = y_const_1 * sin(phi_1); // y2
						tri_list[num_triangles * 10 + 5] = -z_val_1;   // z2
						tri_list[num_triangles * 10 + 6] = x_const_0 * cos(phi_0);  // x3
						tri_list[num_triangles * 10 + 7] = y_const_0 * sin(phi_0); // y3
						tri_list[num_triangles * 10 + 8] = -z_val_0;   // z3
						tri_list[num_triangles * 10 + 9] = 010002.0 + is_reflective;   // exact 0, CSG 1, refractive, 3-1 invisible
						num_triangles++;

						tri_list[num_triangles * 10 + 0] = x_const_1 * cos(phi_1);   // x1
						tri_list[num_triangles * 10 + 1] = y_const_1 * sin(phi_1); // y1
						tri_list[num_triangles * 10 + 2] = -z_val_1; // z1
						tri_list[num_triangles * 10 + 3] = x_const_0 * cos(phi_0);   // x2
						tri_list[num_triangles * 10 + 4] = y_const_0 * sin(phi_0); // y2
						tri_list[num_triangles * 10 + 5] = -z_val_0;   // z2
						tri_list[num_triangles * 10 + 6] = x_const_0 * cos(phi_1);  // x3
						tri_list[num_triangles * 10 + 7] = y_const_0 * sin(phi_1); // y3
						tri_list[num_triangles * 10 + 8] = -z_val_0;   // z3
						tri_list[num_triangles * 10 + 9] = 010001.0 + is_reflective;   // exact 0, CSG 1, refractive, 3-1 invisible
						num_triangles++;
					}
				}
			}

			data[10] = num_triangles; /* how many we actually wrote out */
			return 0;
			}

		/* iterate to exact solution given starting point */
		case 2:
			{
			/*
			We are giving starting data, and must compute the distance to the actual surface.
			We also need to compute the normal vector at the point.

			This is simple iteration. All surfaces must be described as:
			F(x,y,z) = 0
			We will also need Fx = dF/dx, Fy = dF/dy, and Fz = dF/dz
			For the cylindrical face, we have set the triangles to exact = 1
			F  = R*R - X*X - Y*Y
			Fx = -2X
			Fy = -2Y
			Fz = 0.0

			then compute
			Fp = Fx*l + Fy*m + Fz*n where l, m, and n are the ray cosines

			the propagation distance is just
			delt = -F/Fp;
			keep a running total on the position
			t += delt;

			increment the ray coordinates
			x += l*delt;
			y += m*delt;
			z += n*delt;

			repeat until delt is small!

			The data ZEMAX sends is formatted as follows:
			data[2], data[3], data[4] = x, y, z
			data[5], data[6], data[7] = l, m, n
			data[8] = exact code

			*/
				switch ((int)data[8])
				{
					default:
						/* no need to iterate, assume exact solution is on flat triangle face */
						break;
					case 1:
					{
						int loop;
						double x, y, z, l, m, n, t, delt, F, Fx, Fy, Fz, Fp;

						x = data[2];
						y = data[3];
						z = data[4];
						l = data[5];
						m = data[6];
						n = data[7];
						t = 0.0;
						delt = 100.0; /* any big number to start */
						loop = 0; /* loop is to prevent infinite loops, which will HANG ZEMAX! */

						while (fabs(delt) > 1E-10 && loop < 200)
						{
							loop++;

							F = x * x / ( aa * aa ) + y * y / ( bb * bb ) + z * z / ( cc * cc) - 1;
							Fx = 2.0 * x / (aa * aa);
							Fy = 2.0 * y / (bb * bb);
							Fz = 2.0 * z / (cc * cc);
							Fp = Fx * l + Fy * m + Fz * n;

							delt = -F / Fp;
							t += delt;

							x += l * delt;
							y += m * delt;
							z += n * delt;
						}

						/* we have converged, hopefully */
						if (fabs(delt) > 1E-8)
						{
							/* assume ray misses */
							return -1;
						}

						/* normalize Fx, Fy, and Fz to get the normal vector */
						Fp = sqrt(Fx * Fx + Fy * Fy + Fz * Fz);
						if (Fp == 0.0) Fp = 1.0; /* this is not possible for a real surface, but better safe than sorry! */
						Fx /= Fp;
						Fy /= Fp;
						Fz /= Fp;
						data[10] = t;
						data[11] = Fx;
						data[12] = Fy;
						data[13] = Fz;
						return 0;
					}
					break;
				}
			}
			break;

		/* coating data */
		case 3:
			return -1;

		/* safe data */
		case 4:
			/* set safe parameter data values the first time the DLL is initialized */
			aa = 1.0;
			bb = 1.0;
			cc = 1.0;
			n_theta = 10;
			n_phi = 10;
			data[101] = aa;
			data[102] = bb;
			data[103] = cc;
			data[104] = (double)n_theta;
			data[105] = (double)n_phi;
			data[106] = 1.0;
			data[107] = 0.0;
			return 0;
		}

	/* we did not recognize the request */
	return -1;
   }

int __declspec(dllexport) APIENTRY UserParamNames(char *data) {
	/* this function returns the name of the parameter requested */
	int i;
	i = atoi(data);
	strcpy_s(data, 1, ""); // blank means unused

	/* negative numbers or zero mean names for coat/scatter groups */
	if (i ==  0) strcpy_s(data, 15, "Ellipsoid Face");
	if (i == 1) strcpy_s(data, 7, "a");
	if (i == 2) strcpy_s(data, 7, "b");
	if (i == 3) strcpy_s(data, 9, "c");
	if (i == 4) strcpy_s(data, 8, "# theta");
	if (i == 5) strcpy_s(data, 6, "# phi");
	if (i == 6) strcpy_s(data, 13, "Is a volume?");
	if (i == 7) strcpy_s(data, 15, "Is reflective?");

	return 0;
}


#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/*
Written by Kenneth E. Moore
January 17, 2003
Modified December 16, 2003 to support the "diffractive flag" on data[12].
Modified July 29, 2005 to support the "safe values" code = 3.
*/

// modified KEM 4-12-2006 to include coating placeholders, safe data moved to code = 4


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

/*

This DLL models a half cylinder. The parameters are the length and radius of the cylinder.
The number of facets to use in approximating the curve of the cylinder is also a user definable parameter.

*/

int __declspec(dllexport) APIENTRY UserObjectDefinition(double *data, double *tri_list) {
	int code, n_theta, n_phi;
	double aa, bb, cc;

	aa = data[101];
	bb = data[102];
	cc = data[103];

	n_theta = (int)data[104];
	n_phi = (int)data[105];

	/* do some basic error trapping and handling */
	if (aa <= 0.0) aa = 1.0;
	if (bb <= 0.0) bb = 1.0;
	if (cc <= 0.0) cc = 1.0;
	if (n_theta < 4) n_theta = 10;
	if (n_phi < 3) n_phi = 10;

	/* what we do now depends upon what was requested */
	code = (int) data[1];

	switch(code) {
		/* basic data */
		case 0:
			/* Compute the total number of triangular facets used to render and trace this object */
			/* we need N on each end cap, 2N along the face, and 2 on the flat back */
			/* put this value in data[10] */
			data[10] = 2 * n_phi;
			
			/* is this object a solid? put 1 in data[11], use 0 if shell */
			data[11] = 1;

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
			double x_const, y_const, phi_0, phi_1;

			num_triangles = 0;

			/* Coordinates of poles */
			double north_pole[3] = { 0.0, 0.0, cc };
			double south_pole[3] = { 0.0, 0.0, -cc };

			/* Z coordinate of cap layers */
			double north_cap_z = cc * cos( PI / n_theta );
			double south_cap_z = -north_cap_z;

			x_const = aa * sin(PI / n_theta);
			y_const = bb * sin(PI / n_theta);

			/* Create the caps with triangles */
			for (int ii = 0; ii < n_phi; ii++) {
				phi_0 = (double)ii / n_phi * 2 * PI;
				phi_1 = ((double)ii + 1) / n_phi * 2 * PI;

				tri_list[num_triangles * 10 + 0] = north_pole[0];   // x1
				tri_list[num_triangles * 10 + 1] = north_pole[1]; // y1
				tri_list[num_triangles * 10 + 2] = north_pole[2]; // z1
				tri_list[num_triangles * 10 + 3] = x_const * cos( phi_0 );   // x2
				tri_list[num_triangles * 10 + 4] = y_const * sin( phi_0 ); // y2
				tri_list[num_triangles * 10 + 5] = north_cap_z;   // z2
				tri_list[num_triangles * 10 + 6] = x_const * cos( phi_1 );  // x3
				tri_list[num_triangles * 10 + 7] = y_const * sin( phi_1 ); // y3
				tri_list[num_triangles * 10 + 8] = north_cap_z;   // z3
				tri_list[num_triangles * 10 + 9] = 000000.0;   // exact 0, CSG 1, refractive, 3-1 invisible
				num_triangles++;

				tri_list[num_triangles * 10 + 0] = south_pole[0];   // x1
				tri_list[num_triangles * 10 + 1] = south_pole[1]; // y1
				tri_list[num_triangles * 10 + 2] = south_pole[2]; // z1
				tri_list[num_triangles * 10 + 3] = x_const * cos(phi_0);   // x2
				tri_list[num_triangles * 10 + 4] = y_const * sin(phi_0); // y2
				tri_list[num_triangles * 10 + 5] = south_cap_z;   // z2
				tri_list[num_triangles * 10 + 6] = x_const * cos(phi_1);  // x3
				tri_list[num_triangles * 10 + 7] = y_const * sin(phi_1); // y3
				tri_list[num_triangles * 10 + 8] = south_cap_z;   // z3
				tri_list[num_triangles * 10 + 9] = 000000.0;   // exact 0, CSG 1, refractive, 3-1 invisible
				num_triangles++;
			}

			/* Create next layer (temporary) */



			data[10] = num_triangles; /* how many we actually wrote out */
			return 0;
			}

		/* iterate to exact solution given starting point */
		case 2:
			{
			
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

	return 0;
}


/*
 * rdm_grid4D.c - routines to read 4D grid data from an MTV-format.
 *
 * The data is assumed to be sandwiched between $DATA=xxx lines
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "CNplot.h"

int CNmtv_read_grid4D_data();
static void read_4dgd_options();

static CNdouble def_mval = {0.0, 0};

/* 
 * Read data in 4D grid format
 * Return an EOF if such is found during the read; otherwise return 0
 */
int CNmtv_read_grid4D_data(Dptr,
                           dataID,dataname,filename,fp,lineno,header,verbose)
CNdatasetptr *Dptr;          /* The dataset pointer                  */
int          *dataID;        /* Data ID                              */
char         *dataname;      /* Dataset name                         */
char         *filename;      /* The name of the file/pipe source     */
FILE         *fp;            /* The file/pipe source                 */
int          *lineno;        /* Current line number                  */
char         *header;        /* Header string                        */
int          verbose;        /* Verbosity Flag                       */
{
   CNannotptr         annothead=NULL, annottail=NULL;
   CNcontstepptr      cstephead=NULL, csteptail=NULL;
   CNviewptr          view_pr;
   CNdataset_property ds_property;
   CNplotset_property pt_property;
   char        line[BUFSIZ];
   int         header_found  = CN_FALSE;
   int         invalid_grid  = CN_FALSE;
   int         newgrid       = CN_TRUE;
   int         read_xgrid    = CN_FALSE;
   int         read_ygrid    = CN_FALSE;
   int         read_zgrid    = CN_FALSE;
   int         xgrid_filled  = CN_FALSE;
   int         ygrid_filled  = CN_FALSE;
   int         zgrid_filled  = CN_FALSE;
   int         tgrid_filled  = CN_FALSE;
   int         i, len, nx, ny, nz, npts=0;
   int         xcount=0, ycount=0, zcount=0, tcount=0;
   double      values[MAX_VALS];
   double      *xgrid_arr=NULL, *ygrid_arr=NULL, *zgrid_arr = NULL;
   double      *tgrid_arr=NULL;
   double      z=0.0, dx, dy, dz;
   CNdouble    xmin, xmax, ymin, ymax, zmin, zmax, rnx, rny, rnz;
   CNdouble    xgrid, ygrid, zgrid;
   int         binary;

   /* Initialize */
   *Dptr     = NULL;
   xmin      = def_mval;
   xmax      = def_mval;
   ymin      = def_mval;
   ymax      = def_mval;
   zmin      = def_mval;
   zmax      = def_mval;
   rnx       = def_mval;
   rny       = def_mval;
   rnz       = def_mval;
   xgrid     = def_mval;
   ygrid     = def_mval;
   zgrid     = def_mval;

   /* Print info */
   (void) fprintf(stdout,"\n   Reading GRID4D data...(line %d)\n",*lineno);

   /* Set the view parameters */
   view_pr = CNcreate_view();

   /* Set the properties of the plotset */
   CNset_default_plotset_property(&pt_property);

   /* Set the properties of the dataset */
   CNset_default_dataset_property(&ds_property);

   /*
    * This expects the xgrid, followed by y-grid, z-grid and t-array info
    */

   /* Keep on reading until a "$" is encountered */
   while (!header_found && CNgetucline(fp, line, lineno, BUFSIZ) != EOF) {

      /*EMPTY*/
      if (((len=strlen(line)) == 0) || (len==1 && line[0]=='\n')) {
         /* Ignore newlines */
         ;

      /*EMPTY*/
      } else if (line[0] == '#') {
         /* Comment */
         ;

      } else if (line[0] == '$') {
         /* Header */
         line[0] = ' ';
         (void) strcpy(header, line);
         header_found = CN_TRUE;

      } else if (line[0] == '@') {
         /* Annotation */
         line[0] = ' ';
         CNparse_annotation_line(line, BUFSIZ,
                                 &annothead, &annottail,
                                 verbose);

      } else if (line[0] == '%') {
         /* Option  */
         line[0] = ' ';
         read_4dgd_options(line,view_pr,&pt_property,&ds_property,
                         &xmin,&xmax,&ymin,&ymax,&zmin,&zmax,&rnx,&rny,&rnz,
                         &xgrid,&ygrid,&zgrid,&binary,verbose);
         if (xgrid.def) read_xgrid = (int) xgrid.val;
         if (ygrid.def) read_ygrid = (int) ygrid.val;
         if (zgrid.def) read_zgrid = (int) zgrid.val;
         if ((ds_property.contours) && ((ds_property.flag1 & CNctrlevel)!=0))
         CNset_contstep_levels(&cstephead, &csteptail, &ds_property);

         /* Binary format - data follows on next line */
         if (binary) {

            /*EMPTY*/
            if (invalid_grid) {
               /* Skip */
               ;
            } else if (read_xgrid && !xgrid_filled) {
               /* Read the x-grid values */

               /* Set the number of grid points */
               invalid_grid = CNmtv_check_points(&rnx,&nx,"nx",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid) {
                  if (verbose)
                     (void) fprintf(stdout,
                     "\n   Reading %d x-values...line=%d\n",nx,*lineno);

                  /* Read the data  */
                  xgrid_arr = CNmtv_read_binary_dbl_array(fp,nx);
                  if (xgrid_arr != NULL) {
                     newgrid      = CN_TRUE;
                     xgrid_filled = CN_TRUE;
                     xcount       = nx;
                  } else {
                     invalid_grid = CN_TRUE;
                  }
               }
            } else if (read_ygrid && !ygrid_filled) {
               /* Read the y-grid values */

               /* Set the number of grid points */
               invalid_grid = CNmtv_check_points(&rny,&ny,"ny",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid) {
                  if (verbose)
                     (void) fprintf(stdout,
                     "\n   Reading %d y-values...line=%d\n",ny,*lineno);

                  /* Read the data  */
                  ygrid_arr = CNmtv_read_binary_dbl_array(fp,ny);
                  if (ygrid_arr != NULL) {
                     newgrid      = CN_TRUE;
                     ygrid_filled = CN_TRUE;
                     ycount       = ny;
                  } else {
                     invalid_grid = CN_TRUE;
                  }
               }
            } else if (read_zgrid && !zgrid_filled) {
               /* Read the z-grid values */

               /* Set the number of grid points */
               invalid_grid = CNmtv_check_points(&rnz,&nz,"nz",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid) {
                  if (verbose)
                     (void) fprintf(stdout,
                     "\n   Reading %d z-values...line=%d\n",nz,*lineno);

                  /* Read the data  */
                  zgrid_arr = CNmtv_read_binary_dbl_array(fp,nz);
                  if (zgrid_arr != NULL) {
                     newgrid      = CN_TRUE;
                     zgrid_filled = CN_TRUE;
                     zcount       = nz;
                  } else {
                     invalid_grid = CN_TRUE;
                  }
               }
            } else if (!tgrid_filled) {
               /* Read the t-grid values */

               /* Set the number of grid points */
               invalid_grid = CNmtv_check_points(&rnx,&nx,"nx",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid)
               invalid_grid = CNmtv_check_points(&rny,&ny,"ny",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid)
               invalid_grid = CNmtv_check_points(&rnz,&nz,"nz",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid) {
                  if (verbose)
                     (void) fprintf(stdout,
                     "\n   Reading %d x %d x %d t-values...line=%d\n",
                     nx,ny,nz,*lineno);

                  /* Read the data  */
                  tgrid_arr = CNmtv_read_binary_dbl_array(fp,nx*ny*nz);
                  if (tgrid_arr != NULL) {
                     newgrid      = CN_TRUE;
                     tgrid_filled = CN_TRUE;
                     tcount       = nx*ny*nz;
                  } else {
                     invalid_grid = CN_TRUE;
                  }
               }
            }
         }
      } else {
         /* Must be ascii data */ 

         /*EMPTY*/
         if (invalid_grid) {
            /* Skip */
            ;

         } else if (read_xgrid && !xgrid_filled) {
            /* Read the x-grid values */
            if (newgrid) {
               /* Set the number of grid points */
               invalid_grid = CNmtv_check_points(&rnx,&nx,"nx",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid) {
                  if (verbose)
                     (void) fprintf(stdout,
                     "\n   Reading %d x-values...line=%d\n",nx,*lineno);

                  /* allocate space for the data in a 1D array */
                  xgrid_arr = CNcreate_1D_double_array(nx);

                  /* Start collecting points in this grid */
                  newgrid  = CN_FALSE;
                  xcount   = 0;
               }
            }

            /* Read all the points off the line and store */
            if (xgrid_arr != NULL && !invalid_grid) {
               CNmtv_read_line(line,values,MAX_VALS,&npts);
               CNmtv_filter_array(values,&npts,xgrid_arr,&xcount,nx);
               if (xcount >= nx) {
                  xgrid_filled = CN_TRUE;
                  newgrid      = CN_TRUE;
               } 
            }

         } else if (read_ygrid && !ygrid_filled) {
            /* Read the y-grid values */
            if (newgrid) {
               /* Set the number of grid points */
               invalid_grid = CNmtv_check_points(&rny,&ny,"ny",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid) {
                  if (verbose)
                     (void) fprintf(stdout,
                     "\n   Reading %d y-values...line=%d\n",ny,*lineno);

                  /* allocate space for the data in a 1D array */
                  ygrid_arr = CNcreate_1D_double_array(ny);

                  /* Start collecting points in this grid */
                  newgrid  = CN_FALSE;
                  ycount   = 0;
               }
            }

            /* Read all the points off the line and store */
            if (ygrid_arr != NULL && !invalid_grid) {
               CNmtv_read_line(line,values,MAX_VALS,&npts);
               CNmtv_filter_array(values,&npts,ygrid_arr,&ycount,ny);
               if (ycount >= ny) {
                  ygrid_filled = CN_TRUE;
                  newgrid      = CN_TRUE;
               } 
            }

         } else if (read_zgrid && !zgrid_filled) {
            /* Read the z-grid values */
            if (newgrid) {
               /* Set the number of grid points */
               invalid_grid = CNmtv_check_points(&rnz,&nz,"nz",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid) {
                  if (verbose)
                     (void) fprintf(stdout,
                     "\n   Reading %d z-values...line=%d\n",nz,*lineno);

                  /* allocate space for the data in a 1D array */
                  zgrid_arr = CNcreate_1D_double_array(nz);

                  /* Start collecting points in this grid */
                  newgrid  = CN_FALSE;
                  zcount   = 0;
               }
            }

            /* Read all the points off the line and store */
            if (zgrid_arr != NULL && !invalid_grid) {
               CNmtv_read_line(line,values,MAX_VALS,&npts);
               CNmtv_filter_array(values,&npts,zgrid_arr,&zcount,nz);
               if (zcount >= nz) {
                  zgrid_filled = CN_TRUE;
                  newgrid      = CN_TRUE;
               } 
            }

         } else if (!tgrid_filled) {
            /* Read the t-grid values */
            if (newgrid) {
               /* Set the number of grid points */
               invalid_grid = CNmtv_check_points(&rnx,&nx,"nx",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid)
               invalid_grid = CNmtv_check_points(&rny,&ny,"ny",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid)
               invalid_grid = CNmtv_check_points(&rnz,&nz,"nz",
                                                 "GRID4D  ",*lineno);
               if (!invalid_grid) {
                  if (verbose)
                     (void) fprintf(stdout,
                     "\n   Reading %d x %d x %d t-values...line=%d\n",
                     nx,ny,nz,*lineno);

                  /* allocate space for the data in a 1D array */
                  tgrid_arr = CNcreate_1D_double_array(nx*ny*nz);

                  /* Start collecting points in this grid */
                  newgrid  = CN_FALSE;
                  tcount   = 0;
               }
            }

            /* Read all the points off the line and store */
            if (tgrid_arr != NULL && !invalid_grid) {
               CNmtv_read_line(line,values,MAX_VALS,&npts);
               CNmtv_filter_array(values,&npts,tgrid_arr,&tcount,nx*ny*nz);
               if (tcount >= nx*ny*nz) {
                  tgrid_filled = CN_TRUE;
                  newgrid      = CN_TRUE;
               } 
            }
         }
      }
   }

   /* Now create the dataset */
   if (tgrid_arr != NULL && !invalid_grid) {

      /* Check the x array */
      if (read_xgrid && !xgrid_filled) {
         (void) fprintf(stderr,"   warning (line %3d) GRID4D  : ",*lineno);
         (void) fprintf(stderr,
                "Too few x-points read in - need %d values.\n",nx);
         dx = 1.0;
         if (xcount > 1) dx = xgrid_arr[xcount-1] - xgrid_arr[xcount-2];
         (void) fprintf(stderr,
                        "   ******* Assuming uniform intervals of %g",dx);
         (void) fprintf(stderr,
                        " for the remaining grid points\n");
         for (i=xcount; i<nx; i++) {
            z = xgrid_arr[xcount-1] + (i+1-xcount)*dx;
            CNset_1D_double_array_value(xgrid_arr,i,nx,&z); 
         }

      } else if (!read_xgrid) {
         /* Uniform grid */
         if (!xmin.def || !xmax.def) {  
            (void) fprintf(stderr,"   warning (line %3d) GRID4D  : ",*lineno);
            (void) fprintf(stderr,"xmin and/or xmax has not been specified.\n");
            xmin.val = 0.0;
            xmax.val = xmin.val + nx-1;
            (void) fprintf(stderr,
                   "   ******* Setting xmin=%g xmax=%g\n",xmin.val,xmax.val);
         }
         /* allocate space for the data in a 1D array */
         xgrid_arr = CNcreate_1D_double_array(nx);

         /* Fill the array */
         dx = (xmax.val - xmin.val)/(double)(nx-1);
         for (i=0; i<nx; i++) {
            z = xmin.val + i*dx;
            CNset_1D_double_array_value(xgrid_arr,i,nx,&z); 
         }
      }

      /* Check the y array */
      if (read_ygrid && !ygrid_filled) {
         (void) fprintf(stderr,"   warning (line %3d) GRID4D  : ",*lineno);
         (void) fprintf(stderr,
                "Too few y-points read in - need %d values.\n",ny);
         dy = 1.0;
         if (ycount > 1) dy = ygrid_arr[ycount-1] - ygrid_arr[ycount-2];
         (void) fprintf(stderr, 
                        "   ******* Assuming uniform intervals of %g",dy);
         (void) fprintf(stderr, 
                        " for the remaining grid points\n");
         for (i=ycount; i<ny; i++) {
            z = ygrid_arr[ycount-1] + (i+1-ycount)*dy;
            CNset_1D_double_array_value(ygrid_arr,i,ny,&z); 
         }

      } else if (!read_ygrid) {
         /* Uniform grid */
         if (!ymin.def || !ymax.def) {  
            (void) fprintf(stderr,"   warning (line %3d) GRID4D  : ",*lineno);
            (void) fprintf(stderr,"ymin and/or ymax has not been specified.\n");
            ymin.val = 0.0;
            ymax.val = ymin.val + ny-1;
            (void) fprintf(stderr,
                   "   ******* Setting ymin=%g ymax=%g\n",ymin.val,ymax.val);
         }
         /* allocate space for the data in a 1D array */
         ygrid_arr = CNcreate_1D_double_array(ny);

         /* Fill the array */
         dy = (ymax.val - ymin.val)/(double)(ny-1);
         for (i=0; i<ny; i++) {
            z = ymin.val + i*dy;
            CNset_1D_double_array_value(ygrid_arr,i,ny,&z); 
         }
      }

      /* Check the z array */
      if (read_zgrid && !zgrid_filled) {
         (void) fprintf(stderr,"   warning (line %3d) GRID4D  : ",*lineno);
         (void) fprintf(stderr,
                "Too few z-points read in - need %d values.\n",nz);
         dz = 1.0;
         if (zcount > 1) dz = zgrid_arr[zcount-1] - zgrid_arr[zcount-2];
         (void) fprintf(stderr,
                        "   ******* Assuming uniform intervals of %g",dz);
         (void) fprintf(stderr,
                        " for the remaining grid points\n");
         for (i=zcount; i<nz; i++) {
            z = zgrid_arr[zcount-1] + (i+1-zcount)*dz;
            CNset_1D_double_array_value(zgrid_arr,i,nz,&z); 
         }

      } else if (!read_zgrid) {
         /* Uniform grid */
         if (!zmin.def || !zmax.def) {  
            (void) fprintf(stderr,"   warning (line %3d) GRID4D  : ",*lineno);
            (void) fprintf(stderr,"zmin and/or zmax has not been specified.\n");
            zmin.val = 0.0;
            zmax.val = zmin.val + nz-1;
            (void) fprintf(stderr,
                   "   ******* Setting zmin=%g zmax=%g\n",zmin.val,zmax.val);
         }
         /* allocate space for the data in a 1D array */
         zgrid_arr = CNcreate_1D_double_array(nz);

         /* Fill the array */
         dz = (zmax.val - zmin.val)/(double)(nz-1);
         for (i=0; i<nz; i++) {
            z = zmin.val + i*dz;
            CNset_1D_double_array_value(zgrid_arr,i,nz,&z); 
         }
      }

      /* If the t-array is not full, print warning */
      if (!tgrid_filled) {
         (void) fprintf(stderr,"   warning (line %3d) GRID4D  : ",*lineno);
         (void) fprintf(stderr,
               "Too few points! Need %d x %d x %d points (Found %d)\n",
                nx,ny,nz,tcount);
         (void) fprintf(stderr,
                "   ******* Assuming 0-values for the remaining grid points\n");
         z = 0.0;
         for (i=tcount; i<nx*ny*nz; i++)
            CNset_1D_double_array_value(tgrid_arr,i,nx*ny*nz,&z); 
      }

      /* rearrange the data into a volumetric grid */
      *Dptr = CNget_volumetric_grid_data(filename, dataname,
                        xgrid_arr, ygrid_arr, zgrid_arr, tgrid_arr,
                        nx, ny, nz, *dataID);
      if ((*Dptr) != NULL) (*dataID)++;

      if ((*Dptr) != NULL) {
         /* Save the annotations */
         (*Dptr)->annothead = annothead;
         (*Dptr)->annottail = annottail;

         /* Set the contfill flag */
         (void) CNparse_dataset_property(&((*Dptr)->data_pr),
                                         "contfill","True",0);

         /* Apply the view options to the dataset */
         CNset_view_property((*Dptr)->view_pr,view_pr);

         /* Apply the plotset options to the dataset */
         CNset_plotset_property(&((*Dptr)->plot_pr),&pt_property);

         /* Apply the dataset options to the dataset */
         CNset_dataset_property(&((*Dptr)->data_pr),&ds_property);

         /* Apply the contsteps */
         if (cstephead != NULL) {
         (*Dptr)->cstephead = cstephead;
         (*Dptr)->csteptail = csteptail;
         }

         /*
          * Select the contour step size
          * This is done ONLY if the steps have NOT been specified individually
          */
         if ((*Dptr)->data_pr.stepmethod != CN_USERDEFN)
         CNselect_contour_step(&((*Dptr)->data_pr.cmin),
                               &((*Dptr)->data_pr.cmax),
                               (*Dptr)->data_pr.stepmethod,
                               &((*Dptr)->data_pr.cstep),
                               &((*Dptr)->data_pr.nsteps),
                               (*Dptr)->data_pr.logzstep,
                               &((*Dptr)->cstephead), &((*Dptr)->csteptail),
                               verbose);

         /* Print out the curve */
         if (verbose) CNprint_dataset((*Dptr),0);

      }
   }

   /* Failed */
   if ((*Dptr)==NULL) {
      CNdelete_contstep_list(&cstephead, &csteptail);
      CNdelete_annotation_list(&annothead, &annottail);
   }

   /* Reset the property structures */
   CNdelete_plotset_property_fields(&pt_property);
   CNdelete_dataset_property_fields(&ds_property);

   /* Free the view structure */
   CNdelete_view(view_pr);

   /* Free the arrays */
   if ((*Dptr) == NULL) {
   if (xgrid_arr != NULL) CNfree_1D_double_array(xgrid_arr);
   if (ygrid_arr != NULL) CNfree_1D_double_array(ygrid_arr);
   if (zgrid_arr != NULL) CNfree_1D_double_array(zgrid_arr);
   if (tgrid_arr != NULL) CNfree_1D_double_array(tgrid_arr);
   }

   /* Print out info */
   if ((*Dptr) != NULL) 
   (void) fprintf(stdout,"   GRID4D   : Read %d x %d x %d points\n",nx,ny,nz);
   else
   (void) fprintf(stdout,"   GRID4D   : Format error - couldn't read data!\n");

   /* return */
   if (!header_found) 
      return(EOF);
   else    
      return(header_found);
}


/* 
 * Read a line and apply grid options to a dataset
 */
static void read_4dgd_options(header,view_pm,pt_prop,ds_prop,
                            xmin,xmax,ymin,ymax,zmin,zmax,nx,ny,nz,
                            xgrid,ygrid,zgrid,binary,vbs)
char               *header;
CNviewptr          view_pm;
CNplotset_property *pt_prop;
CNdataset_property *ds_prop;
CNdouble           *xmin,*xmax,*ymin,*ymax,*zmin,*zmax,*nx,*ny,*nz;
CNdouble           *xgrid,*ygrid,*zgrid;
int                *binary;
int                vbs;
{
   char *argtbl[CN_MAXWORDS], *valtbl[CN_MAXWORDS];
   char newheader[CN_MAXCHAR];
   int  nargs = 0, nvals = 0;
   int  argfound, i;
   short sval;

   /* Initialize */
   *binary = CN_FALSE;

   /* CNparse_line wants "command arg=val arg=val" so create a new header */
   (void) sprintf(newheader, "datafile %s",header);

   /* Get the argument-value pairs from the line */
   if (CNparse_line(newheader, CN_MAXCHAR,
                    &nargs, argtbl, CN_MAXWORDS,
                    &nvals, valtbl, CN_MAXWORDS)) {

      /* Go thru the arguments and find specialized matches */
      i = 0;
      while (i < nargs) {

         /* Go thru the arguments and find a match */
         argfound = CN_TRUE;

         if (strcmp(argtbl[i],"xmin")==0) {
            xmin->val = atof(valtbl[i]);
            xmin->def = CN_TRUE;
            if (vbs)
            (void)fprintf(stdout,"   GRID4D  : %-14s= %g\n","xmin",xmin->val);

         } else if (strcmp(argtbl[i],"xmax")==0) {
            xmax->val = atof(valtbl[i]);
            xmax->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %g\n","xmax",xmax->val);

         } else if (strcmp(argtbl[i],"ymin")==0) {
            ymin->val = atof(valtbl[i]);
            ymin->def = CN_TRUE;
            if (vbs)
            (void)fprintf(stdout,"   GRID4D  : %-14s= %g\n","ymin",ymin->val);

         } else if (strcmp(argtbl[i],"ymax")==0) {
            ymax->val = atof(valtbl[i]);
            ymax->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %g\n","ymax",ymax->val);

         } else if (strcmp(argtbl[i],"zmin")==0) {
            zmin->val = atof(valtbl[i]);
            zmin->def = CN_TRUE;
            if (vbs)
            (void)fprintf(stdout,"   GRID4D  : %-14s= %g\n","zmin",zmin->val);

         } else if (strcmp(argtbl[i],"zmax")==0) {
            zmax->val = atof(valtbl[i]);
            zmax->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %g\n","zmax",zmax->val);

         } else if (strcmp(argtbl[i],"nx")==0) {
            nx->val = atof(valtbl[i]);
            nx->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %d\n","nx",(int)nx->val);

         } else if (strcmp(argtbl[i],"ny")==0) {
            ny->val = atof(valtbl[i]);
            ny->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %d\n","ny",(int)ny->val);

         } else if (strcmp(argtbl[i],"nz")==0) {
            nz->val = atof(valtbl[i]);
            nz->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %d\n","nz",(int)nz->val);

         } else if (strcmp(argtbl[i],"xgrid")==0) {
            CNassign_boolean_keyword(&sval,valtbl[i],"xgrid",0);
            xgrid->val = sval;
            xgrid->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %d\n","xgrid",
                          (int)xgrid->val);

         } else if (strcmp(argtbl[i],"ygrid")==0) {
            CNassign_boolean_keyword(&sval,valtbl[i],"ygrid",0);
            ygrid->val = sval;
            ygrid->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %d\n","ygrid",
                          (int)ygrid->val);

         } else if (strcmp(argtbl[i],"zgrid")==0) {
            CNassign_boolean_keyword(&sval,valtbl[i],"zgrid",0);
            zgrid->val = sval;
            zgrid->def = CN_TRUE;
            if (vbs) 
            (void)fprintf(stdout,"   GRID4D  : %-14s= %d\n","zgrid",
                          (int)zgrid->val);

         } else if ( (strcmp(argtbl[i],"binary")==0) || 
                     ( (strcmp(argtbl[i],"format")==0) &&
                       (strcmp(valtbl[i],"binary")==0) ) ) {
            *binary = CN_TRUE;
            if (vbs)
            (void)fprintf(stdout,"   GRID4D  : %-14s= %s\n","Binary","ON");

         } else {
            argfound = CN_FALSE;
         }

         /* Reset the table if a match was found; otherwise increment index */
         CNdownshift(argfound,&i,argtbl,valtbl,&nargs,&nvals);
      }

      /* Look for plotset/dataset/curve arguments */
      for (i=0; i<nargs; i++) {
         if (!CNparse_view_property       (view_pm,argtbl[i],valtbl[i],vbs))
           if (!CNparse_plotset_property  (pt_prop,argtbl[i],valtbl[i],vbs))
             if (!CNparse_dataset_property(ds_prop,argtbl[i],valtbl[i],vbs))
                (void) fprintf(stderr,"   warning : Invalid option \"%s=%s\"\n",
                               argtbl[i],valtbl[i]);
      }

      /* Clear the tables */
      CNfreewords(&nargs, argtbl);
      CNfreewords(&nvals, valtbl);
   }
}



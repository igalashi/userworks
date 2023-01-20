#include <iomanip>
#include <time.h>
#include <sys/time.h>

#include "MessageUtil.h"
#include "Reporter.h"
#include "HulStrTdcFilter.h"
#include "utility/HexDump.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0') 
const double	Byte2MB		     = 1.0e-6;
const double	ns2s		     = 1.0e-9;
const double	ns2ms		     = 1.0e-6;
const double	s2ms		     = 1.0e+3;
const double	s2ns		     = 1.0e+9;
const double	Deg2Rad		     = acos(-1.)/180.;
const double	Rad2Deg		     = 180./acos(-1.);
/* define the width of time window in ns */
const uint64_t	time_window_width    = 50;
const int	numb_of_layer	     = 12;
// X				     = 12 fibers, U = V = 10 fibers
const int	numb_of_ch_per_layer = 12;
const int	numb_of_ch	     = 128;
const int	hits_max	     = 12;
const int	track_max	     = 100;
const int	n_x_ch		     = 12;
const int	n_u_ch		     = 10;
const int	n_v_ch		     = 10;

/* within each layer, fiber shifted by half cell */
/*  [1][3][5][][]... */
/* [0][2][4][][].....*/
/* this lookup table accepts only single hit per layer */
/* one can extend it to accepting multi-hits by indexing fired wire id */
struct pos{
  bool status; /* if found == true; not found == false */
  float x;
  float y;
};
/* return position based on X, U, V fiber id*/
pos pos_tbl[ n_x_ch ][ n_u_ch ][ n_v_ch ];
void init_pos_tbl()
{
  for ( int i = 0; i < n_x_ch; i++ )
    {
      for ( int j = 0; j < n_u_ch; j++ )
	{
	  for ( int k = 0; k < n_v_ch; k++ )
	    {
	      pos_tbl[ i ][ j ][ k ].status = false;
	      pos_tbl[ i ][ j ][ k ].x = -999.;
	      pos_tbl[ i ][ j ][ k ].y = -999.;
	    }
	}
    }
  /* y = ax + b */
  float ang_u = (30.0 + 90.0) * Deg2Rad; //rot_u is defined against X, +90 for Y direction
  float rot_u = 30.0 * Deg2Rad; //rot_u is defined against X, +90 for Y direction
  float a_u = 1.0 * tan( ang_u );
  float ang_v = (-30.0 + 90.0) * Deg2Rad ; /* rot_v is defined against X, +90 for Y direction */
  float rot_v = -30.0 * Deg2Rad; /* rot_v is defined against X, +90 for Y direction */
  float a_v = 1.0 * tan( ang_v );
  float x_uv[ n_u_ch ][ n_v_ch ];
  float y_uv[ n_u_ch ][ n_v_ch ];
  for ( int id_u = 0; id_u < n_u_ch; id_u++ )
    {
      for ( int id_v = 0; id_v < n_v_ch; id_v++ )
  	{
  	  float b_u = id_u * 0.5 / sin( rot_u );
  	  float b_v = id_v * 0.5 / sin( rot_v );
  	  float x = (b_u - b_v)/(a_v - a_u);
  	  float y = a_u * x + b_u;
	  x_uv[ id_u ][ id_v ] = x;
	  y_uv[ id_u ][ id_v ] = y;
  	  /* printf( "id_u = %d, id_v = %d\n", id_u, id_v ); */
  	  /* printf( "x = %f, y = %f \n", x, y ); */
  	  //printf("\033[%d;%dH%s\n", int(x), int(y), "*");
  	  //	  printf("\033[%d;%dH%s\n", 1, 3, "*");
  	  // printf("\033[%d;%dH%s\n", id_u, id_v, "*");
	  /* find x with shortest distance */
	  float x_tmp = 999.;
	  int id_x_cal = -1;
	  for ( int id_x = 0; id_x < n_x_ch; id_x++ )
	    {
	      if ( fabs( x - (float)id_x * 0.5 ) <= x_tmp )
	  	{
	  	  x_tmp = fabs( x - (float)id_x * 0.5 );  /* coincide with X layer */
	  	  //printf( "id_x = %d, delta_x = %.6f \n", id_x, x_tmp );
	  	}
	      else
	  	{
	  	  id_x_cal = id_x - 1;
	  	  //printf( " sloved id_x_cal = %d, delta_x = %f \n", id_x_cal, x_tmp );
	  	  pos_tbl[ id_x_cal ][ id_u ][ id_v ].status = true;
	  	  pos_tbl[ id_x_cal ][ id_u ][ id_v ].x = x;
	  	  pos_tbl[ id_x_cal ][ id_u ][ id_v ].y = y;
	  	  break;
	  	}
	    }
	  /* /\* create table based on prefixed distance of ~1mm *\/ */
	  /* int id_x_cal = -1; */
	  /* for ( int id_x = 0; id_x < n_x_ch; id_x++ ) */
	  /*   { */
	  /*     if ( fabs( x - (float)id_x * 0.5 ) <= 0.5 ) */
	  /* 	{ */
	  /* 	  id_x_cal = id_x; */
	  /* 	  //printf( " sloved id_x_cal = %d, delta_x = %f \n", id_x_cal, x_tmp ); */
	  /* 	  pos_tbl[ id_x_cal ][ id_u ][ id_v ].status = true; */
	  /* 	  pos_tbl[ id_x_cal ][ id_u ][ id_v ].x = x; */
	  /* 	  pos_tbl[ id_x_cal ][ id_u ][ id_v ].y = y; */
	  /* 	} */
	  /*   } */
  	}
    }
  // display calculated fiber configuration
  // TFile *root_file = new TFile( "tmp.root", "RECREATE" );
  // root_file->cd();
  // TH2F *h2 = new TH2F("h2","uv position", 1000, -1, 6, 1000, -6, 6 );
  // for (int i=0; i<n_u_ch; i++)
  //   {
  //     for (int j=0; j<n_v_ch; j++)
  // 	{
  // 	  h2->Fill( x_uv[i][j], y_uv[i][j], 1. );
  // 	}
  //   }
  // TCanvas* c = new TCanvas("c", "c", 400, 800);
  // h2->Draw();
  // for (int i=0; i<n_x_ch; i++)
  //   {
  //     TLine *line = new TLine( i*0.5, -6, i*0.5, 6 );
  //     line->SetLineColor(kRed);
  //     line->Draw("same");
  //   }
  // for ( int id_u = 0; id_u < n_u_ch; id_u++ )
  //   {
  //     float b_u = id_u * 0.5 / sin( rot_u );
  //     float x1 = -1.0 ;
  //     float y1 = a_u * x1 + b_u;
  //     float x2 = 6.0 ;
  //     float y2 = a_u * x2 + b_u;
  //     TLine *line = new TLine( x1, y1, x2, y2 );
  //     line->SetLineColor(kBlue);
  //     line->Draw("same");
  //   }
  // for ( int id_v = 0; id_v < n_v_ch; id_v++ )
  //   {
  //     float b_v = id_v * 0.5 / sin( rot_v );
  //     float x1 = -1.0 ;
  //     float y1 = a_v * x1 + b_v;
  //     float x2 = 6.0 ;
  //     float y2 = a_v * x2 + b_v;
  //     TLine *line = new TLine( x1, y1, x2, y2 );
  //     line->SetLineColor(kGreen);
  //     line->Draw("same");
  //   }
  // c->Write();
  // root_file->Write();
  // root_file->Close();
} //end of lookup table for fiber tracker

// new counter map by Y. Ma and K. Shirotori
// 20180217
int counter_map[ 128 ][ 2 ]; // 128 HUL ch: layer, fiber
void init_counter_map() // read complete ch correspondence
{
  /* XUV XUV XUV XUV */
  /* 123 456 789 101112 */
  /* for ( int i = 0; i < 128; i++ ) */
  /* 	{ */
  /* 	    printf( "ch = %3d, layer = %2d, fiber = %2d \n", */
  /* 		    i, counter_map[ i ][ 0 ], counter_map[ i ][ 1 ] ); */
  /* 	} */
  /* getchar(); */
  FILE	*fp	= fopen ("CounterMap0.txt", "r");
  int	 line	= 0;
  char	 type[2];
  int	 layer	= 0;
  int	 u_d	= 0;
  int	 fiber	= 0;
  int	 hul_ch = 0;
  while ( EOF != fscanf ( fp, "%d %s %d %d %d %d",
			  &line, type, &layer, &u_d, &fiber, &hul_ch ) )
    {
      /* printf ("line = %d, type = %s, layer = %d, u_d = %d, fiber = %d, hul_ch = %d \n",  */
      /* 	    line, type, layer, u_d, fiber, hul_ch ); */
      counter_map[ hul_ch - 1 ][ 0 ] = layer - 1;
      if ( u_d == 1 ) // 0, 2, 4, ..., 10
	{
	  counter_map[ hul_ch - 1 ][ 1 ] = fiber * 2 - 2;
	}
      else // 1, 3, 5, ..., 9
	{
	  counter_map[ hul_ch - 1 ][ 1 ] = fiber * 2 -1;
	}
    }
}

/* wrapper function tp return a time stamp in [ms] */
double get_time_stamp()
{
  static struct timespec start_local;
  clock_gettime( CLOCK_REALTIME, &start_local );
  return start_local.tv_sec * s2ms + start_local.tv_nsec * ns2ms;
}

// to encode wanted data into HUL data format, used in emulator
void encode_hul_raw_word(hul_raw_word_t *hul_raw_word, uint32_t head, uint32_t rsv,
                         uint32_t tot, uint32_t type, uint32_t ch, uint32_t tdc)
{
  hul_raw_word->data[4] |= (head & 0xF) << 4;
  hul_raw_word->data[4] |= (rsv & 0x1) << 3;
  hul_raw_word->data[4] |= (tot >> 5) & 0x7;
  // std::cout << "encode_hul_raw_word:" << std::endl;
  // std::cout << "(head & 0xF) << 4 = " << std::dec << std::bitset< 8 > ( ( head & 0xF ) << 4 ) << std::endl;
  // std::cout << "hul_raw_word->data[4] = " << std::dec << std::bitset< 8 > ( hul_raw_word->data[4] ) << std::endl;
  hul_raw_word->data[3] |= (tot & 0x1F) << 3;
  hul_raw_word->data[3] |= (type & 0x3) << 1;
  hul_raw_word->data[3] |= (ch >> 5) & 0x1;
  //  std::cout << "hul_raw_word[3] = " << std::dec << std::bitset< 8 > ( hul_raw_word[3] ) << std::endl;
  hul_raw_word->data[2] |= (ch & 0x1F) << 3;
  hul_raw_word->data[2] |= (tdc >> 16) & 0x7;
  //  std::cout << "hul_raw_word[2] = " << std::dec << std::bitset< 8 > ( hul_raw_word[2] ) << std::endl;
  hul_raw_word->data[1] |= (tdc >> 8) & 0xFF;
  //  std::cout << "hul_raw_word[1] = " << std::dec << std::bitset< 8 > ( hul_raw_word[1] ) << std::endl;
  hul_raw_word->data[0] |= (tdc >> 0) & 0xFF;
  // std::cout << "hul_raw_word[0] = " << std::dec << std::bitset< 8 > ( hul_raw_word[0] ) << std::endl;
  // std::cout << "hul_raw_word = " << std::dec << std::bitset< 64 > ( *((uint64_t*) hul_raw_word) ) << std::endl;
  //std::cout << "hul_raw_word[4] = " << std::dec << unsigned( hul_raw_word[4] ) << std::endl;
}

const  uint32_t tf_header_size			  = 24;	// [3 * 64 bit = 24Byte]
const  uint32_t stf_header_size			  = 32;	// [4 * 64 bit = 32Byte]
const  uint32_t hul_raw_word_size		  = 5;	// 40 bit = 5 Byte
const  uint32_t num_hul_mod			  = 6;
static uint32_t	current_hul_id			  = 0;
static uint64_t	current_mus_window[ num_hul_mod ] = {0};
static uint16_t fired_layer_id[ num_hul_mod ]	  = {0};	// hold fired layer id by setting ith bit
static uint64_t heart_beat_counter[ num_hul_mod ] = {0};

// to decode HUL data format and assing ch/layer with counter map
void decode_hul_raw_word(hul_raw_word_t *hul_raw_word, hul_decode_word_t *hul_decode_word)
{
  // should implement a new counter map HERE!!
  uint32_t head = 0;
  uint32_t rsv = 0;
  uint32_t tot = 0;
  uint32_t type = 0;
  uint32_t ch = 0;
  uint32_t tdc = 0;
  uint64_t heart_beat = 0;
  head |= (hul_raw_word->data[4] >> 4) & 0xF;
  // std::cout << "(hul_raw_word->data[4] >> 4) = " << std::bitset< 8 > (hul_raw_word->data[4] >> 4) << std::endl;
  // std::cout << "(hul_raw_word->data[4] >> 4) & 0xF = " << std::bitset< 8 > ( (hul_raw_word->data[4] >> 4) & 0xF ) << std::endl;
  // std::cout << "head = " << std::bitset< 8 > (head)  << std::endl;
  // std::cout << "head = " << std::hex << head << std::endl;
  if( head == 0xD ) // data word
    {
      rsv |= (hul_raw_word->data[4] >> 3) & 0x1;
      tot |= (hul_raw_word->data[4] & 0x7) << 5;
      tot |= (hul_raw_word->data[3] >> 3) & 0x1F;
      type |= (hul_raw_word->data[3] >> 1) & 0x3;
      ch |= (hul_raw_word->data[3] & 0x1) << 5;
      ch |= (hul_raw_word->data[2] >> 3) & 0x1F;
      tdc |= (hul_raw_word->data[2] & 0x7) << 16;
      tdc |= (hul_raw_word->data[1] & 0xFF) << 8;
      tdc |= (hul_raw_word->data[0] & 0xFF);
      // convert this raw ch + current hul_id into fiber/counter/chamber ch and layer
      hul_decode_word->ch = ch; 
      hul_decode_word->tot = tot;
      hul_decode_word->time_stamp = heart_beat_counter[ current_hul_id ] * 500 * 1000 + tdc;
    }
  else if( (head == 0xF) || (head == 0xE) ) // heart beat word
    {
      heart_beat |= (hul_raw_word->data[1] & 0xFF) << 8;
      heart_beat |= (hul_raw_word->data[0] & 0xFF);
      heart_beat_counter[ current_hul_id ] = heart_beat;
      // return a "magic" word in time_stamp to inform the occurence of heart beat word
      hul_decode_word->time_stamp = 0; 
    }
  else if( head == 0x4 ) // spill end found
    {
      //std::cout << "spill end found ..." << std::endl;
      // asign an impossible time stamp value as magic word to inform spill end
      hul_decode_word->time_stamp = 500 * 1000; 
    }
  else
    {
      std::cout << "unknown hul raw word !!" << std::endl;
    }
}

//______________________________________________________________________________
highp::e50::
HulStrTdcFilter::HulStrTdcFilter()
  : FairMQDevice()
{
}

int filter_counter = 0;
//______________________________________________________________________________
std::vector< hul_decode_word_t >
highp::e50::
HulStrTdcFilter::ApplyFilter(const std::vector<std::vector<char>>& inputData)
{
  // merged data holder
  std::vector< hul_decode_word_t > mgd_dat; 
  // data from each hul
  std::vector< hul_decode_word_t > hul_mod_dat[ num_hul_mod ]; 
  // tmp holder for mus time window
  std::vector< hul_decode_word_t > hul_mod_dat_tmp[ num_hul_mod ]; 

  double spill_time_sta;
  double spill_time_stp;
  spill_time_sta = get_time_stamp();  
  // first level vector: TF header + ( STF header + Heart beat + TDC + Spill end ) x 6
  // second level vector for header: magic + evt_id + ...
  // second level vector for HUL: hul data word, 5 Byte ...
  std::cout << " ApplyFilter Called ...  " << std::dec << filter_counter << " times " << std::endl;
  std::cout << " parts size = " << inputData.size() << std::endl; // number of first level vector
  for (int i=0; i<inputData.size(); ++i){ // loop into first level vector
    if ( (i % 5000) == 0 )
      {
	std::cout << "i = " << i << std::endl;
      }
    const auto& data = inputData.at(i); // get ith first level vector
    // tell the current data type with its size
    // hul word = n*5Byte, will not be messed up with headers
    if ( data.size() == tf_header_size )
      {
  	std::cout << "TF header found" << std::endl;
  	std::cout << "data["<< i << "] = " << std::hex << *((uint64_t *)(&data.at( 0*sizeof(uint64_t) ))) << std::endl;
  	std::cout << "data["<< i << "] = " << std::dec << *((uint64_t *)(&data.at( 1*sizeof(uint64_t) ))) << std::endl;
  	std::cout << "data["<< i << "] = " << std::dec << *((uint64_t *)(&data.at( 2*sizeof(uint64_t) ))) << std::endl;
      }
    else if ( data.size() == stf_header_size )
      {
	uint64_t stf_fem_id = *((uint64_t *)(&data.at( 2*sizeof(uint64_t) )));
  	std::cout << "STF header found" << std::endl;
  	std::cout << "stf_magic = " << std::hex << *((uint64_t *)(&data.at( 0*sizeof(uint64_t) ))) << std::endl;
  	std::cout << "stf_evt_id = " << std::dec << *((uint64_t *)(&data.at( 1*sizeof(uint64_t) ))) << std::endl;
  	std::cout << "stf_fem_id = " << std::dec << *((uint64_t *)(&data.at( 2*sizeof(uint64_t) ))) << std::endl;
  	std::cout << "stf_tot_len = " << std:: dec << *((uint64_t *)(&data.at( 3*sizeof(uint64_t) ))) << std::endl;
	// assign current hul module id at STF header
	// this approach should be fine as far as STF header always comes before HUL data
	if ( stf_fem_id == 3232238084 )
	  current_hul_id = 0;
	else if ( stf_fem_id == 3232238085 )
	  current_hul_id = 1;
	else if ( stf_fem_id == 3232238086 )
	  current_hul_id = 2;
	else if ( stf_fem_id == 3232238087 )
	  current_hul_id = 3;
	else if ( stf_fem_id == 3232238088 )
	  current_hul_id = 4;
	else if ( stf_fem_id == 3232238089 )
	  current_hul_id = 5;
	else
	  {
	    std::cout << "unknown hul mod id !!" << std::dec << stf_fem_id << std::endl;
	  }
      }
    else // loop on hul raw data
      { 
	for (int j = 0; j < data.size()/sizeof(hul_raw_word_t); j++) 
	  {
	    hul_raw_word_t *tmp_raw_word = (hul_raw_word_t *)&data.at( j*sizeof(hul_raw_word_t) );
	    hul_decode_word_t tmp_decode_word;
	    memset( &tmp_decode_word, 0, sizeof(hul_decode_word_t) );
	    decode_hul_raw_word( tmp_raw_word, &tmp_decode_word );
	    // avoid filling heart beat and spill end word into decode data buffer
	    if ( tmp_decode_word.time_stamp == 0 ) // heart beat
	      {
		//std::cout << "heart beat found = " << heart_beat_counter[ current_hul_id ] << std::endl;
	      }
	    else if ( tmp_decode_word.time_stamp == 500 * 1000 ) // spill end
	      {
		std::cout << "spill end found " << std::endl;
		heart_beat_counter[ current_hul_id ] = 0;
		current_mus_window[ current_hul_id ] = 0;
		fired_layer_id[ current_hul_id ] = 0;
	      }
	    else // normal hul tdc data 
	      {
		
		{ // W/O filter
		  // fill decode buffer W/O filtering
		  hul_mod_dat[ current_hul_id ].push_back( tmp_decode_word );
		  std::cout << "current_hul_id = " << current_hul_id << std::endl;
		  std::cout << "ch = " << std::hex << tmp_decode_word.ch 
			    << " layer = " << std::hex << unsigned(tmp_decode_word.layer)
			    << "tot = " << std::hex << unsigned(tmp_decode_word.tot) 
			    << " time_stamp = " << std::hex << tmp_decode_word.time_stamp << std::endl;		  
		} // end of W/O filter
		
		// { // W/ filter
		//   // only apply filter to fiber tracker, hul_id: 0, 1, 2, 3
		//   if ( current_hul_id <= 3 ) 
		//     {
		//       // cut on multiplicity within 1mus time window
		//       // over kill by this method should be ~ few percent ( 10 ns (?) fiber signal fluctuation vs 1mus)
		//       // cast ns into mus
		//       uint64_t tmp_mus_window = static_cast<std::int64_t>( tmp_decode_word.time_stamp / 1000. );
		//       // a new mus time window started, filter events belong to old window
		//       if ( tmp_mus_window > current_mus_window[ current_hul_id ] ) 
		// 	{ 
		// 	  if ( __builtin_popcount( fired_layer_id[ current_hul_id ] ) == 3 )
		// 	    {
		// 	      hul_mod_dat[ current_hul_id ].insert( hul_mod_dat[ current_hul_id ].end(),
		// 						    hul_mod_dat_tmp[ current_hul_id ].begin(),
		// 						    hul_mod_dat_tmp[ current_hul_id ].end());
		// 	    }
		// 	  current_mus_window[ current_hul_id ] = tmp_mus_window;
		// 	  hul_mod_dat_tmp[ current_hul_id ].clear();
		// 	  fired_layer_id[ current_hul_id ] = 0;
		// 	  // push the first decode word back to tmp buffer
		// 	  hul_mod_dat_tmp[ current_hul_id ].push_back( tmp_decode_word );
		// 	}
		//       else // still in the same time window, fill into tmp buffer and save multiplicity info
		// 	{
		// 	  // need a counter map
		// 	  // int layer_id = counter_map[ channel ][ 0 ];
		// 	  // int ch_id = counter_map[ channel ][ 1 ];
		// 	  // fired_layer_id |= 1UL << layer_id;
		// 	  hul_mod_dat_tmp[ current_hul_id ].push_back( tmp_decode_word );
		// 	}
		//     }
		//   else // for chamber and counter, no filter applied
		//     {
		//       hul_mod_dat[ current_hul_id ].push_back( tmp_decode_word );
		//       std::cout << "current_hul_id = " << current_hul_id << std::endl;
		//       std::cout << "ch = " << std::hex << tmp_decode_word.ch 
		// 		<< " layer = " << std::hex << unsigned(tmp_decode_word.layer) << std::endl;
		//       std::cout << "tot = " << std::hex << unsigned(tmp_decode_word.tot) 
		// 		<< " time_stamp = " << std::hex << tmp_decode_word.time_stamp << std::endl;
		//     }
		// }// end of W/ filter
		
	      }
	  }
      }
  } // end of decode loop on inputData
  
  // merge decoded hul data into mdg_dat; hul_mod_dat is already sorted (as TDC stream)
  int mid_pos = 0;
  for (int i = 0; i < num_hul_mod; i++ )
    {
      //std::cout << "hul_mod_dat[ i ].size() = " << hul_mod_dat[ i ].size() << std::endl;
      if ( hul_mod_dat[ i ].size() == 0 )
	continue;
      mgd_dat.insert( mgd_dat.end(), hul_mod_dat[ i ].begin(), hul_mod_dat[ i ].end() );
      if ( (mid_pos > 0) && (mid_pos < mgd_dat.size()) )
  	{
  	  std::inplace_merge( mgd_dat.begin(),
  			      mgd_dat.begin() + mid_pos,
  			      mgd_dat.end(),
  			      [] (const hul_decode_word_t &a, const hul_decode_word_t &b)
  			      {
  				return a.time_stamp < b.time_stamp;
  			      });
  	}
      mid_pos += hul_mod_dat[ i ].size();
    }
  for ( int i = 0; i < mgd_dat.size(); i++ )
    {
      std::cout << "mgd_dat.at( " << i << " )" << mgd_dat.at( i ).time_stamp << std::endl;
    }
  
  filter_counter++;  
  
  // insert a "magic word" to book mark the end of the current data trunk
  hul_decode_word_t tmp;
  tmp.ch = 9;
  tmp.layer = 9;
  tmp.tot = 9;
  tmp.time_stamp = 9;
  mgd_dat.push_back( tmp );
  return std::move( mgd_dat );

// static uint64_t tdc_bgn = 0; /* hold beginning tdc */
// static uint64_t tdc_pre[ numb_of_ch ] = {0}; 
// static uint16_t filtered_layer_id = 0;
// /* ch id after up/down layer coincidence */
// static uint16_t fired_ch_id[ numb_of_layer ] = {0}; 
// static uint16_t fired_ch_id_all[ numb_of_layer ] = {0}; 
// /* hold candidates for both even and odd id */
// static uint16_t fired_ch_id_cnd[ numb_of_layer ][ 2 ]; 
// static uint16_t fired_ch_id_tst[ numb_of_layer ][ 2 ]; 
// static int filtered_mul[ numb_of_layer ]; 
// static int filtered_id[ numb_of_layer ][ hits_max ]; 
// static int id_x[ hits_max ] = {0}; 
// static int id_u[ hits_max ] = {0}; 
// static int id_v[ hits_max ] = {0}; 
// /* hit position from XUV */
// static float pos_mul[ 4 ];
// static float x[ 4 ][ hits_max ];
// static float y[ 4 ][ hits_max ];
// static float x0[ 2 ][ hits_max ];
// static float y0[ 2 ][ hits_max ];
// static int channel = 0;
  // apply filter to mrged data
  // use hul_dat_len[] and hul_buf_pos[] to merge HUL branches together  
  // else { // vector for hul buffer
  //   // for (int j = 0; j < data.size(); j++) 
  //   // 	{
  //   // 	  std::cout << "data["<< i << "] = " << std::setfill('0') << std::setw(2) << std::hex << unsigned( (uint8_t)data.at( j ) ) << std::endl;
  //   // 	}
  //   for (int j = 0; j < data.size()/sizeof(hul_data_word_t); j++) // loop into second level vector
  // 	{
  // 	  // clear data for spill end
  // 	  hul_data_word_t *tmp_data_word = (hul_data_word_t *)&data.at( j*sizeof(hul_data_word_t) );
  // 	  // for (int k = 0; k < sizeof(hul_data_word_t); k++) 
  // 	  //   {
  // 	  //     std::cout << "data["<< i << "] = " << std::setfill('0') << std::setw(2) << std::hex << unsigned( (uint8_t)tmp_data_word->data[k] ) << std::endl;
  // 	  //   }
  // 	  head = 0, rsv = 0, tot = 0, type = 0, ch = 0, tdc = 0;
  // 	  decode_hul_data_word(tmp_data_word, &head, &rsv, &tot, &type, &ch, &tdc);
  // 	  std::cout << "head = " << std::hex << head << " tdc = " << std::hex << tdc << std::endl;
  // 	  // std::vector<int> ch_container[ numb_of_layer ];
  // 	  // int layer_id = counter_map[ channel ][ 0 ];
  // 	  // int ch_id = counter_map[ channel ][ 1 ];
  // 	  // fired_layer_id |= 1UL << layer_id;
  // 	  // fired_ch_id[ layer_id ] |= 1UL << ch_id;
  // 	  // /* if time window is filled, then process it */
  // 	  // /* this is to make sure that time cluster is sepatated by >=time_window_width */
  // 	  // if ( tdc_bgn <= time_stamp - time_window_width ) 
  // 	  //   { /* time window full, run filter and reset tdc_bgn */
  // 	  // 	tdc_bgn = time_stamp;
  // 	  // 	h_layer_multi->Fill( __builtin_popcount( fired_layer_id ) );
  // 	  // 	double dec_time_sta = 0;
  // 	  // 	double dec_time_stp = 0;
  // 	  // 	dec_time_sta = get_time_stamp();
  // 	  // 	if ( __builtin_popcount( fired_layer_id ) == 12 )
  // 	  // 	//if ( __builtin_popcount( fired_layer_id ) >= 2 )
  // 	  // 	  {/*begin of multiplicity cut*/
  // 	  // 	    /* int fired_layer_mul = __builtin_popcount( fired_layer_id ); */
  // 	  // 	    /* for ( int l_tmp = 0; l_tmp < fired_layer_mul; l_tmp++ ) */
  // 	  // 	    /*   { */
  // 	  // 	    /* 	int l = __builtin_ffs( fired_layer_id ) - 1; */
  // 	  // 	    /* 	fired_layer_id &= ~( 1UL << l ); */
  // 	  // 	    /* 	// eliminate duplicated ch within same time cluster */
  // 	  // 	    /* 	std::sort( ch_container[ l ].begin(), ch_container[ l ].end() ); */
  // 	  // 	    /* 	/\* auto last = std::unique( ch_container[ l ].begin(), ch_container[ l ].end() ); *\/ */
  // 	  // 	    /* 	/\* ch_container[ l ].erase( last, ch_container[ l ].end() );  *\/ */
  // 	  // 	    /* 	ch_container[ l ].erase( */
  // 	  // 	    /* 				std::unique( ch_container[ l ].begin(), ch_container[ l ].end() ), */
  // 	  // 	    /* 				ch_container[ l ].end() */
  // 	  // 	    /* 				 ); */
  // 	  // 	    /* 	filtered_mul[ l ] = ch_container[ l ].size(); */
  // 	  // 	    /* 	for ( int hit = 0; hit < filtered_mul[ l ] - 1; hit++ ) */
  // 	  // 	    /* 	  { */
  // 	  // 	    /* 	    // check if neighbouring fiber has hit */
  // 	  // 	    /* 	    if ( ch_container[ l ][ hit ]+1 == ch_container[ l ][ hit+1 ] ) */
  // 	  // 	    /* 	      { */
  // 	  // 	    /* 		int id_tmp = ch_container[ l ][ hit ]; */
  // 	  // 	    /* 		filtered_id[ l ][ hit ] = id_tmp; */
  // 	  // 	    /* 		h_filtered_id[ l ]->Fill( id_tmp ); */
  // 	  // 	    /* 	      } */
  // 	  // 	    /* 	  } */
  // 	  // 	    /*   } */
  // 	  // 	    for ( int l = 0; l < numb_of_layer; l++ )
  // 	  // 	      {
  // 	  // 	    	//printf("layer %d: \n", l);
  // 	  // 	    	/* //printf("Leading text "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(a)); */
  // 	  // 	    	/* printf("odd bit map: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\n", */
  // 	  // 	    	/*        BYTE_TO_BINARY( fired_ch_id_odd[ l ]>>8 ), BYTE_TO_BINARY( fired_ch_id_odd[ l ] )); */
  // 	  // 	    	/* printf("eve bit map: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\n", */
  // 	  // 	    	/*        BYTE_TO_BINARY( fired_ch_id_eve[ l ]>>8 ), BYTE_TO_BINARY( fired_ch_id_eve[ l ] )); */
  // 	  // 	    	/* fired_ch_id[ l ] = fired_ch_id_odd[ l ] & fired_ch_id_eve[ l ]; */
  // 	  // 	    	/* printf("and bit map: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\n", */
  // 	  // 	    	/*        BYTE_TO_BINARY( fired_ch_id[ l ]>>8 ), BYTE_TO_BINARY( fired_ch_id[ l ] )); */
  // 	  // 	    	// pair plane coincidence
  // 	  // 	    	//fired_ch_id[ l ] = fired_ch_id_cnd[ l ][ 0 ] & fired_ch_id_cnd[ l ][ 1 ];
  // 	  // 	    	// use filled fired_ch_id directly w/o pair plance coincidence
  // 	  // 	    	if( l == 0 )
  // 	  // 	    	  h_ch_multi->Fill( __builtin_popcount( fired_ch_id[ l ] ) );
  // 	  // 	    	if ( __builtin_popcount( fired_ch_id[ l ] ) > 0 )
  // 	  // 	    	  filtered_layer_id |= 1UL << l;
  // 	  // 	      }
  // 	  // 	    //if ( __builtin_popcount( filtered_layer_id ) == 12 )
  // 	  // 	      /* if ( (filtered_layer_id >> 0 & 0x7) == 7 || */
  // 	  // 	      /* 	 ((filtered_layer_id >> 3) & 0x7) == 7 || */
  // 	  // 	      /* 	 ((filtered_layer_id >> 6) & 0x7) == 7 || */
  // 	  // 	      /* 	 ((filtered_layer_id >> 9) & 0x7) == 7 ) */
  // 	  // 	    /* if ( (filtered_layer_id >> 0 & 0x7) >= 3 || */
  // 	  // 	    /* 	 ((filtered_layer_id >> 3) & 0x7) >= 3 || */
  // 	  // 	    /* 	 ((filtered_layer_id >> 6) & 0x7) >= 3 || */
  // 	  // 	    /* 	 ((filtered_layer_id >> 9) & 0x7) >= 3 ) */
  // 	  // 	      {
  // 	  // 		//h_tdc_all_fine_aft->Fill( time_stamp );
  // 	  // 		int filtered_layer_mul = __builtin_popcount( filtered_layer_id );
  // 	  // 		for ( int l = 0; l < filtered_layer_mul; l++ )
  // 	  // 		  {
  // 	  // 		    int l_tmp = __builtin_ffs( filtered_layer_id ) - 1;
  // 	  // 		    filtered_layer_id &= ~( 1UL << l_tmp );
  // 	  // 		    filtered_mul[ l_tmp ] = __builtin_popcount( fired_ch_id[ l_tmp ] );
  // 	  // 		    for ( int hit = 0; hit < filtered_mul[ l_tmp ]; hit++ )
  // 	  // 		      {
  // 	  // 			int id_tmp = __builtin_ffs( fired_ch_id[ l_tmp ] ) - 1;
  // 	  // 			fired_ch_id[ l_tmp ] &= ~( 1UL << id_tmp ); /* reset bit to avoid double count */
  // 	  // 			filtered_id[ l_tmp ][ hit ] = id_tmp;
  // 	  // 			h_filtered_id[ l_tmp ]->Fill( id_tmp );
  // 	  // 		      }
  // 	  // 		  }
  // 	  // 		/* for ( int m = 0; m < filtered_mul[ 0 ]; m++ ) */
  // 	  // 		/*   { */
  // 	  // 		/*     for ( int n = 0; n < filtered_mul[ 3 ]; n++ ) */
  // 	  // 		/* 	{ */
  // 	  // 		/* 	  h_filtered_xx_id->Fill( filtered_id[ 0 ][ m ], filtered_id[ 3 ][ n ]); */
  // 	  // 		/* 	} */
  // 	  // 		/*   } */
  // 	  // 		/* for ( int m = 0; m < filtered_mul[ 1 ]; m++ ) */
  // 	  // 		/*   { */
  // 	  // 		/*     for ( int n = 0; n < filtered_mul[ 4 ]; n++ ) */
  // 	  // 		/* 	{ */
  // 	  // 		/* 	  h_filtered_uu_id->Fill( filtered_id[ 1 ][ m ], filtered_id[ 4 ][ n ]); */
  // 	  // 		/* 	} */
  // 	  // 		/*   } */
  // 	  // 		/* for ( int m = 0; m < filtered_mul[ 2 ]; m++ ) */
  // 	  // 		/*   { */
  // 	  // 		/* 	for ( int n = 0; n < filtered_mul[ 5 ]; n++ ) */
  // 	  // 		/* 	  { */
  // 	  // 		/* 	    h_filtered_vv_id->Fill( filtered_id[ 2 ][ m ], filtered_id[ 5 ][ n ]); */
  // 	  // 		/* 	  } */
  // 	  // 		/*   } */
  // 	  // 		for ( int l = 0; l < numb_of_layer; l = l+3 )
  // 	  // 	      	  {/* begin of tracking */
  // 	  // 	      	    int n_pos = 0;
  // 	  // 	      	    for ( int hit_x = 0; hit_x < filtered_mul[ l ]; hit_x++ )
  // 	  // 	      	      {
  // 	  // 	      		int id_x_tmp = filtered_id[ l ][ hit_x ];
  // 	  // 	      		id_x[ hit_x ] = id_x_tmp;
  // 	  // 	      		for ( int hit_u = 0; hit_u < filtered_mul[ l+1 ]; hit_u++ )
  // 	  // 	      		  {
  // 	  // 	      		    int id_u_tmp = filtered_id[ l+1 ][ hit_u ];;
  // 	  // 	      		    id_u[ hit_u ] = id_u_tmp;
  // 	  // 	      		    for ( int hit_v = 0; hit_v < filtered_mul[ l+2 ]; hit_v++ )
  // 	  // 	      		      {
  // 	  // 	      			int id_v_tmp = filtered_id[ l+2 ][ hit_v ];
  // 	  // 	      			id_v[ hit_v ] = id_v_tmp;
  // 	  // 	      			if ( pos_tbl[ id_x[hit_x] ][ id_u[hit_u] ][ id_v[hit_v] ].status )
  // 	  // 	      			  {
  // 	  // 	      			    /* printf("hit position found: id_x = %d, id_u = %d, id_v = %d, x = %f, y = %f \n", */
  // 	  // 	      			    /*        id_x[hit_x], id_u[hit_u], id_v[hit_v],  */
  // 	  // 	      			    /*        pos_tbl[ id_x[hit_x] ][ id_u[hit_u] ][ id_v[hit_v] ].x,  */
  // 	  // 	      			    /*        pos_tbl[ id_x[hit_x] ][ id_u[hit_u] ][ id_v[hit_v] ].y); */
  // 	  // 	      			    //getchar();
  // 	  // 	      			    h_xy[ l/3 ]->Fill( pos_tbl[ id_x[hit_x] ][ id_u[hit_u] ][ id_v[hit_v] ].x,
  // 	  // 	      					       pos_tbl[ id_x[hit_x] ][ id_u[hit_u] ][ id_v[hit_v] ].y );
  // 	  // 	      			    if ( n_pos < hits_max )
  // 	  // 	      			      {
  // 	  // 	      				x[ l/3 ][ n_pos ] = pos_tbl[ id_x[hit_x] ][ id_u[hit_u] ][ id_v[hit_v] ].x;
  // 	  // 	      				y[ l/3 ][ n_pos ] = pos_tbl[ id_x[hit_x] ][ id_u[hit_u] ][ id_v[hit_v] ].y;
  // 	  // 	      			      }
  // 	  // 	      			    pos_mul[ l/3 ] = n_pos;
  // 	  // 	      			    n_pos++;
  // 	  // 	      			    h_n_pos->Fill( n_pos );
  // 	  // 	      			  }
  // 	  // 	      		      }
  // 	  // 	      		  }
  // 	  // 	      	      }
  // 	  // 	      	  }/* end of tracking */
  // 	  // 	      	for( int m = 0; m < pos_mul[0]; m++ )
  // 	  // 	      	  {
  // 	  // 	      	    for( int n = 0; n < pos_mul[1]; n++ )
  // 	  // 	      	      {
  // 	  // 	      		h_xxp[ 0 ]->Fill( x[0][m], x[1][n] );
  // 	  // 	      		h_yyp[ 0 ]->Fill( y[0][m], y[1][n] );
  // 	  // 	      	      }
  // 	  // 	      	  }
  // 	  // 	      	for( int m = 0; m < pos_mul[2]; m++ )
  // 	  // 	      	  {
  // 	  // 	      	    for( int n = 0; n < pos_mul[3]; n++ )
  // 	  // 	      	      {
  // 	  // 	      		h_xxp[ 1 ]->Fill( x[2][m], x[3][n] );
  // 	  // 	      		h_yyp[ 1 ]->Fill( y[2][m], y[3][n] );
  // 	  // 	      	      }
  // 	  // 	      	  }
  // 	  // 	      	for( int m = 0; m < pos_mul[2]; m++ )
  // 	  // 	      	  {
  // 	  // 	      	    for( int n = 0; n < pos_mul[0]; n++ )
  // 	  // 	      	      {
  // 	  // 	      		h_xxp[ 2 ]->Fill( x[2][m], x[0][n] );
  // 	  // 	      		h_yyp[ 2 ]->Fill( y[2][m], y[0][n] );
  // 	  // 	      	      }
  // 	  // 	      	  }
  // 	  // 		if( pos_mul[2] > 0 )
  // 	  // 		  {
  // 	  // 		    for( int m = 0; m < pos_mul[0]; m++ )
  // 	  // 		      {
  // 	  // 			for( int n = 0; n < pos_mul[1]; n++ )
  // 	  // 			  {
  // 	  // 			    h_xxp[ 3 ]->Fill( x[0][m], x[1][n] );
  // 	  // 			    h_yyp[ 3 ]->Fill( y[0][m], y[1][n] );
  // 	  // 			  }
  // 	  // 		      }
  // 	  // 		  }
  // 	  // 		/* float z4 = 1672.0; */
  // 	  // 		/* float z3 = 1644.0; */
  // 	  // 		/* float z2 = 1349.0; */
  // 	  // 		/* float z1 = 1321.0; */
  // 	  // 		/* int cnt1 = 0; */
  // 	  // 		/* for( int m = 0; m < pos_mul[2]; m++ ) */
  // 	  // 		/*   { */
  // 	  // 		/*     for( int n = 0; n < pos_mul[3]; n++ ) */
  // 	  // 		/*       { */
  // 	  // 		/* 	x0[1][cnt1] = x[3][n] - (x[3][n] - x[2][m])/(z4 - z3)*z4; */
  // 	  // 		/* 	y0[1][cnt1] = y[3][n] - (y[3][n] - y[2][m])/(z4 - z3)*z4; */
  // 	  // 		/* 	cnt1++; */
  // 	  // 		/* 	h_x0y0[1]->Fill( x0[1][cnt1], y0[1][cnt1] ); */
  // 	  // 		/*       } */
  // 	  // 		/*   } */
  // 	  // 		/* int cnt0 = 0; */
  // 	  // 		/* for( int m = 0; m < pos_mul[0]; m++ ) */
  // 	  // 		/*   { */
  // 	  // 		/*     for( int n = 0; n < pos_mul[1]; n++ ) */
  // 	  // 		/*       { */
  // 	  // 		/* 	x0[0][cnt0] = x[1][n] - (x[1][n] - x[0][m])/(z2 - z1)*z2; */
  // 	  // 		/* 	y0[0][cnt0] = y[1][n] - (y[1][n] - y[0][m])/(z2 - z1)*z2; */
  // 	  // 		/* 	cnt0++; */
  // 	  // 		/* 	h_x0y0[0]->Fill( x0[0][cnt0], y0[0][cnt0] ); */
  // 	  // 		/*       } */
  // 	  // 		/*   } */
  // 	  // 	      }/* end of filtered multiplicity cut*/
  // 	  // 	  }/* end of fired multiplicity cut*/			  
  // 	  // 	  for( int ly = 0; ly < numb_of_layer; ly++ )
  // 	  // 	    {
  // 	  // 	      ch_container[ ly ].clear();
  // 	  // 	    }
  // 	  // 	fired_layer_id = 0;
  // 	  // 	filtered_layer_id = 0;
  // 	  // 	memset( fired_ch_id, 0, sizeof(uint16_t) * numb_of_layer);
  // 	  // 	//memset( fired_ch_id_cnd, 0, sizeof(uint16_t) * numb_of_layer * 2);
  // 	  // 	memset( filtered_mul, 0, sizeof(int) * numb_of_layer );
  // 	  // 	memset( filtered_id, 0, sizeof(int) * numb_of_layer * hits_max );
  // 	  // 	/* memset( tdc_info, 0, sizeof(uint64_t) * */
  // 	  // 	/* 	numb_of_layer * numb_of_ch_per_layer * hits_max ); */
  // 	  // 	/* memset( mul_info, 0, sizeof(int) * */
  // 	  // 	/* 	numb_of_layer * numb_of_ch_per_layer); */
  // 	  // 	dec_time_stp = get_time_stamp();
  // 	  // 	h_filter_time->Fill( dec_time_stp, - dec_time_sta );
  // 	  //   }
  // 	  // else
  // 	  //   {
  // 	  // 	tdc_bgn = time_stamp;
  // 	  //   }	  
  // 	}
 
}

//______________________________________________________________________________
bool
highp::e50::
HulStrTdcFilter::HandleData(FairMQParts& parts, int index)
{
  // std::cout << " HandleData Called ...  " << std::dec << handle_counter << " times " << std::endl;
  // std::cout << " index = " << std::dec << index << std::endl;

  // // apply decoder/filter  
  // std::vector<std::vector<char>> inputData;
  // inputData.reserve(parts.Size());
  // for (const auto& m : parts) {
  //   std::vector<char> msg(std::make_move_iterator(reinterpret_cast<char*>(m->GetData())), 
  // 			  std::make_move_iterator(reinterpret_cast<char*>(m->GetData()) + m->GetSize())); 
  //   inputData.emplace_back(std::move(msg));
  // }
  // auto outputData = ApplyFilter(inputData);  
  // FairMQParts outParts;
  // for (const auto& v : outputData) {
  //   auto vv = std::make_unique< hul_decode_word_t >(std::move(v));
  //   auto msg = MessageUtil::NewMessage(*this, std::move(vv));
  //   outParts.AddPart(std::move(msg));
  // }
  
  // // don't do any thing, put a direct dump of header + hul raw word
  std::vector<std::vector<char>> inputData;
  inputData.reserve(parts.Size());
  for (const auto& m : parts) {
    std::vector<char> msg(std::make_move_iterator(reinterpret_cast<char*>(m->GetData())), 
			  std::make_move_iterator(reinterpret_cast<char*>(m->GetData()) + m->GetSize())); 
    inputData.emplace_back(std::move(msg));
  }
  FairMQParts outParts;
  for (const auto& v : inputData) {
    auto vv = std::make_unique< std::vector<char> >(std::move(v));
    auto msg = MessageUtil::NewMessage(*this, std::move(vv));
    outParts.AddPart(std::move(msg));
  }
  
  while (Send(outParts, fOutputChannelName) < 0) {
    LOG(error) << " failed to enqueue filtered data "; 
  }

  return true;
}

//______________________________________________________________________________
void 
highp::e50::
HulStrTdcFilter::Init() 
{
  Reporter::Instance(fConfig);
}

//______________________________________________________________________________
void 
highp::e50::
HulStrTdcFilter::InitTask()
{
  using opt = OptionKey;
  fInputChannelName  = fConfig->GetValue<std::string>(opt::InputChannelName.data());
  fOutputChannelName = fConfig->GetValue<std::string>(opt::OutputChannelName.data());

  OnData(fInputChannelName, &HulStrTdcFilter::HandleData);
}




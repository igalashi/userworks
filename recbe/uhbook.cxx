/*
 *
 */

#ifndef UHBOOK_HXX
#define UHBOOK_HXX

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

class UH1Book {
public:
	UH1Book();	
	UH1Book(const char *, long, double, double);	
	UH1Book(std::string &, long, double, double);	
	virtual ~UH1Book();
	void Fill(double);
	void Fill(double, double);
	void Print();
	void Draw();
	void setTitle(std::string &);
	std::string getTitle();
	long getEntry() {return m_entry;};
	long getBins() {return m_x_bins.size();};
	void Clear();
	void setNbins(long bins) {if (m_entry == 0) {m_x_bins.resize(bins);}};
	long getNbins() {return m_x_bins.size();};
	void setMin(double min) {if (m_entry == 0) {m_x_min = min;}};
	double getMin() {return m_x_min;};
	void setMax(double max) {if (m_entry == 0) {m_x_max = max;}};
	double getMax() {return m_x_max;};
protected:
private:
	void Init(std::string &, long);

	std::string m_title;
	std::vector<double> m_x_bins;
	double m_x_min;
	double m_x_max;
	std::string m_xlabel;
	std::string m_ylabel;
	long m_entry;
	long m_uf;
	long m_of; 
};

#endif

#include <sys/ioctl.h>
#include <unistd.h>

size_t get_terminal_width()
{
	size_t line_length = 0;
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 ) {
		printf("terminal_width  =%d\n", ws.ws_col);
		printf("terminal_height =%d\n", ws.ws_row);
		if ((ws.ws_col > 0) && (ws.ws_col == (size_t)ws.ws_col)) {
			line_length = ws.ws_col;
		}
	} 
	return line_length;
}

UH1Book::UH1Book()
{
	return;
}

UH1Book::UH1Book(const char *title, long bins, double min, double max)
	: m_x_min(min), m_x_max(max)
{
	std::string stitle(title);
	Init(stitle, bins);
	return;
}

UH1Book::UH1Book(std::string &title, long bins, double min, double max)
	: m_x_min(min), m_x_max(max)
{
	Init(title, bins);

	return;
}

void UH1Book::Init(std::string &title, long bins)
{
	m_title = title;
	m_x_bins.resize(bins);
	return;
}

UH1Book::~UH1Book()
{
	return;
}

/*
void UH1Book::setMin(double min)
{
	if (m_entry == 0) {
		m_x_min = min;
	} else {
	}
	return;
}
*/

void UH1Book::Fill(double val)
{
	Fill(val, 1.0);
	return;
}

void UH1Book::Fill(double val, double weight)
{

	//std::cout << "#D2 " << val << " " << m_x_min << " " << m_x_max << std::endl;

	if (val < m_x_min) m_uf++;
	if (val >= m_x_max) m_of++;
	if ((val >= m_x_min) && (val <= m_x_max)) {
		long index = static_cast<long>(
			(val - m_x_min) / (m_x_max - m_x_min) * m_x_bins.size());
		//std::cout << "#D3 " << index << std::endl;
		m_x_bins[index] = m_x_bins[index] + weight;
		m_entry++;
	}

	return;
}

void UH1Book::Clear()
{
	m_x_bins.clear();
	m_x_bins.resize(0);
	m_entry = 0;
	m_uf = 0;
	m_of = 0; 

	return;
}

void UH1Book::Print()
{
	std::cout << "Title: " << m_title << std::endl;
	std::cout << "Entry: " << m_entry << std::endl;
	std::cout << "Over flow: " << m_of << std::endl;
	std::cout << "Under flow: " << m_uf << std::endl;

	return;
}

void UH1Book::Draw()
{
	int tlen = 80;
	int hlen = tlen - 8;

	double vmax = 0;
	for (size_t i = 0 ; i < m_x_bins.size() ; i++) {
		if (vmax < m_x_bins[i]) vmax = m_x_bins[i];
	}

	for (size_t i = 0 ; i < m_x_bins.size() ; i++) {
		int dnum;
		if (vmax < hlen) {
			dnum = static_cast<int>(m_x_bins[i]);
		} else {
			dnum = m_x_bins[i] / vmax * hlen;
		}
		double xindex = ((m_x_max - m_x_min) / m_x_bins.size() * i) + m_x_min;
		std::cout
			<< std::scientific << std::setprecision(1) 
			<< xindex << ":";
		for (int j = 0 ; j < dnum ; j++) std::cout << "#";
		std::cout << std::endl;
	}

	return;
}


#ifdef TEST_MAIN 
#include <random>
#include <sstream>
int main(int argc, char* argv[])
{
	int nentry = 100;
	for (int i = 1 ; i < argc ; i++) {
		//std::string param(argv[i]);
		std::istringstream iss(argv[i]);
		iss >> nentry;
	}


	UH1Book h1("Hello", 30, 0.0, 200.0);

	//std::string title("Hello");
	//UH1Book h1(title, 100, 2.0, 500.0);
	//std::cout << "#D " << h1.getMin() << " " << h1.getMax()
	//	<< " " << h1.getBins() << std::endl;


	std::random_device rnd;
	std::mt19937 engine(rnd());
	std::normal_distribution<> dist(100.0, 20.0);
	for (int i = 0 ; i < nentry ; i++) {
		double val = dist(engine);
		h1.Fill(val);
	}

	h1.Print();
	h1.Draw();

	return 0;
}
#endif

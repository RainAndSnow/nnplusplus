#include "NeuralNet.h"
#include "auxiliary.h"
#include <string.h>
#include <math.h>
#include <fstream>
#include <stdio.h>


using namespace nnplusplus;


ActiveFunction* activefunction_maker (const char *str)
{
	ActiveFunction* p = NULL;
	printf ("str = %s\n", str);
	//std::cout << strcmp ("negation", "negation") << std::endl;
	if (0 == strcmp ("tanh", str)) {
		p = new TanhFunction ();
		if (NULL == p) {
			printf ("can't allocate memory for tanh Function\n");
			return NULL;
		}
		//printf ("negation\n");
	}
	else if (0 == strcmp ("logistic", str)) {
		p = new LogisticSigmodFunction ();
		if (NULL == p) {
			printf ("Can't allocate memory for logistic function\n");
			return NULL;
		}
		//printf ("logistic\n");
	}
	else {
		p = new NullFunction ();
		if (NULL == p) {
			printf ("Can't allocate memory for null function\n");
			return NULL;
		}
	}
	return p;
}


NeuralNet::NeuralNet (int ln, ...) 
	: layer_num (ln)
	, epoch(100)
	, learing_rate(10)
{
	va_list args;
	va_start (args, ln);
	for (int i = 0; i < layer_num; ++i) {
		layer_size.push_back (va_arg (args, int));
	}
	for (int i = 0; i < layer_num - 1; ++i) {
		active_function.push_back (activefunction_maker (va_arg (args, char*)));
	}
	va_end (args);
	for (int i = 0; i < layer_size.size (); ++i) {
		//std::cout << layer_size [i] << " ";
	}
	//std::cout << std::endl;
	std::cout << "active function size : " << active_function.size () << std::endl;
	init_weights ();
	init_basis ();
}


bool NeuralNet::init_weights ()
{
 	int weight_num = 0;
	weights.clear ();
	for (int i = 0; i < layer_size.size () - 1; ++i) {
		weight_num += layer_size [i] * layer_size [i+1];
	}
	//std::cout << "weight number : " << weight_num << std::endl;
	double w0 = 0.01;//1.0 / weight_num;
	for (long i = 0; i < weight_num; ++i) {
		weights.push_back (w0);
	}
	//std::cout << "weight size : " << weights.size () << std::endl;
	return true;
}


bool NeuralNet::init_basis ()
{
	for (int i = 0; i < layer_num - 1; ++i) {
		basis.push_back (0);
	}
	return true;
}


bool NeuralNet::load_training_set (const std::string& train_file, std::vector<std::pair<std::vector<double>, std::vector<double>>>& training_set) 
{
	std::ifstream infile (train_file);
	if (infile.fail ()) {
		printf ("euralNet::load_training_set open file %s error", train_file.c_str ());
		return false;
	}
	std::string line;
	char* pend = NULL;
	std::getline (infile, line);
	training_size = strtol (line.c_str (), &pend, 10);
	std::cout << "training size : " << training_size << std::endl;
	input_num = strtol (pend, &pend, 10);
	std::cout << "input : " << input_num << std::endl;
	output_num = strtol (pend, NULL, 10);
	std::cout << "output : " << output_num << std::endl;
	while (training_set.size () < training_size && !infile.eof ()) {
		std::vector <double> in;
		std::vector <double> out;
		std::getline (infile, line);
		if (input_num < 2) {
			in.push_back (strtod (line.c_str (), NULL));
		}
		else {
			in.push_back (strtod (line.c_str (), &pend));
			for (int i = 1; i < input_num - 1; ++i) {
				in.push_back (strtod (pend, &pend));
			}
			in.push_back (strtod (pend, NULL));
		}
		std::getline (infile, line);
		if (output_num < 2) {
			out.push_back (strtod (line.c_str (), NULL));
		}
		else {
			out.push_back (strtod (line.c_str (), &pend));
			for (int i = 1; i < output_num - 1; ++i) {
				out.push_back (strtod (pend, &pend));
			}
			out.push_back (strtod (pend, NULL));
		}
		training_set.push_back (std::pair<std::vector<double>, std::vector<double>>(in, out));
	}
	if (training_set.size () < training_size) {
		printf ("NeuralNet::load_training_set less of training set\n");
		return false;
	}
	infile.close ();
	for (int i = 0; i < training_set.size (); ++i) {
		for (int ii = 0; ii < training_set [i].first.size (); ++ii) {
			std::cout << training_set [i].first [ii] << " ";
		}
		std::cout << " : ";
		for (int ii = 0; ii < training_set [i].second.size (); ++ii) {
			std::cout << training_set [i].second [ii] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	return true;
}


bool NeuralNet::sum_of_squares_error (const std::vector<double>& out, const std::vector<double>& t, double& error)
{
	//std::cout << " XXXXXXXXXXX " << t.size () << " " << layer_size [layer_num-1] << std::endl;
	if (t.size () != layer_size [layer_num-1]) {
		printf ("NeuralNet::sum_of_squares_error() : wrong target output size\n");
		return false;
	}
	//std::vector<double> out;
	//output (x, out);
	for (int i = 1; i <= t.size (); ++i) {
		error += pow (t[t.size () - i] - out[out.size () - i], 2);
	}
	error /= 2;
	return true;
}


bool NeuralNet::propagation (const std::vector<double>& input, std::vector<double>& output, const int layer)
{
	if (input.size () != layer_size [layer]) { 
		std::cout << input.size () << " " << layer_size [layer] << std::endl; 
		printf ("NeuralNet::propagation() : wrong input size\n");
		return false;
	}
 	if (layer >= layer_size.size () - 1 ) {
		printf ("NeuralNet::propagation () : wrong layer number\n");
		return false;
	}
	//std::cout << active_function.size () << std::endl;
	int base = 0;
	for (int i = 0; i < layer && i < layer_size.size () - 1; ++i) {
		base += layer_size [i] * layer_size [i+1];
	}
	for (int i = 0; i < layer_size [layer+1]; ++i) {
		double ne = 0;
		for (int j = 0; j < layer_size [layer]; ++j) {
			ne += input [j] * weights [base + i * input.size () + j];
		}
		ne += basis [layer];
		output.push_back ((*active_function [layer])(ne));
	}
 	return true;	
}


bool NeuralNet::output (const std::vector<double>& x, std::vector<double>& out)
{
	out.clear (); 
	out = x;
	int w_base = 0;
	int o_base = 0;
	for (int layer = 0; layer < layer_num - 1; ++layer) {
		//if (!propagation (tmp, out, i)) {
		//	printf ("NeuralNet::output() : propagation error\n");
		//}
		//tmp = out;
		if (layer > 0) {
			w_base += layer_size [layer-1] * layer_size [layer];
			o_base += layer_size [layer-1];
		}
		for (int i = 0; i < layer_size [layer+1]; ++i) {
			double ne = 0;
			if (layer < 1) {
				for (int ii = 0; ii < layer_size [layer]; ++ii) {
					ne +=  x[ii] * weights [i * x.size () + ii];
				}
			}
			else {
				for (int ii = 0; ii < layer_size [layer]; ++ii) {
					//std::cout << " == " << o_base + ii << std::endl;
					//std::cout << " === " << w_base + i * layer_size [layer-1] + ii << std::endl;
					ne +=  out[o_base + ii] * weights [w_base + i * layer_size [layer-1] + ii];
				}
			}
			ne += basis [layer];
			out.push_back ((*active_function [layer])(ne));
		}
	}
 	return true;
}


bool NeuralNet::update_weights (const std::vector<double>& t, const std::vector<double>& out)
{
	if (t.size () <= 0) {
		printf ("NeuralNet::compute_local_gradient () : error target vector\n");
		return false;
	}
	int ob = 0, wb = 0;
	std::vector<int> o_base;
	std::vector<int> w_base;
	for (int i = 0; i < layer_num - 1; ++i) {
		o_base.push_back (ob);
		ob += layer_size [i];
		w_base.push_back (wb);
		wb += layer_size [i] * layer_size [i+1];
	}
	//std::cout << "ok\n";
	std::vector<double> delta;
	//输出层
	for (int i = 0; i < layer_size [layer_size.size () - 1]; ++i) {
		delta.push_back (0.0);
		double o = out [o_base [o_base.size () - 1] + i];
		//std::cout << "out " << o_base [o_base.size () - 1] << " " <<  o << std::endl;
		delta[i] = (t [i] - o);
		//std::cout << "delta " << delta << std::endl;
		if ("logisticsigmod" == active_function [active_function.size () - 1]->name ()) {
			//std::cout << "sigmod\n";
			delta [i] *= o * (1 - o);	
		}
	}
	//隐藏层
	for (int i = 0; i < layer_size [layer_size.size () - 2]; ++i) {
		delta.push_back (0.0);
		double o = out [o_base [o_base.size () -1] + i];
		//std::cout << "out " << o_base.size () - 2 + i << " " <<  o << std::endl;
		int delta_index = layer_size [layer_size.size () - 1] + i;
		//std::cout << "d i "  << delta_index << std::endl;
		delta[delta_index] = 0.0;
		for (int ii = 0; ii < layer_size [layer_size.size () - 1]; ++ii) {
			//std::cout << " jfjfj : " <<     ii + i * layer_size [layer_size.size () - 1] + w_base [w_base.size () - 1]<<  " " << weights [ii + i * layer_size [layer_size.size () - 1] + w_base [w_base.size () - 1]]	<< std::endl;
			//std::cout << delta [ii] <<std::endl;
			delta [delta_index] += weights [ii + i * layer_size [layer_size.size () - 1] + w_base [w_base.size () - 1]] * delta [ii];
		}
		if ("logisticsigmod" == active_function [active_function.size () - 1]->name ()) {
			delta [delta_index] *= o * (1 - o); 
		}
		//std::cout << " delta " << delta_index << " " << delta[delta_index] << std::endl;
	}


	for (int i = 0; i < layer_size [layer_size.size () - 1]; ++i) {
		for (int ii = 0; ii < layer_size [layer_size.size () - 2]; ++ii) {
			//std::cout << w_base [w_base.size () -1] << " ------ " << w_base [w_base.size () -1] + i * layer_size [layer_size.size () - 2] + ii << "\n";
			//std::cout <<  " ++++++++++ " << o_base [layer_num - 2] + ii << std::endl;
			//std::cout << ii << std::endl;
			//std::cout << " d " << learing_rate * delta[i] * out [o_base [layer_num - 2] + ii] << std::endl;
			//std::cout << " d " << out [o_base [layer_num - 2] + ii] << std::endl;
			weights [w_base [w_base.size () - 1] + i * layer_size [layer_size.size () - 2] + ii] += learing_rate * delta[i] * out [o_base [layer_num - 2] + ii];
		}
	}
	for (int i = 0; i < layer_size [layer_size.size () - 2]; ++i) {
		int delta_index = layer_size [layer_size.size () - 1] + i;
		for (int ii = 0; ii < layer_size [layer_size.size () - 2]; ++ii) {
			//std::cout << w_base [w_base.size () -2] << " --- " << w_base [w_base.size () -2] + i * layer_size [layer_size.size () - 2] + ii << "\n";
			//std::cout <<  " +++ " << o_base [layer_num - 3] + ii << std::endl;
			//std::cout << " d " << out [o_base [layer_num - 3] + ii] << std::endl;
			//std::cout << " d " << delta[delta_index] << std::endl;
			weights [w_base [w_base.size () - 2] + i * layer_size [layer_size.size () - 2] + ii] += learing_rate * delta[delta_index] * out [o_base [layer_num - 3] + ii];
		}
	}
	for (int i = 0; i < weights.size (); ++i) {
		std::cout << weights [i] << " ";
	}
	std::cout<<std::endl;
	return true;
}


bool NeuralNet::train_step (double& e, const std::vector<double>& x, const std::vector<double>& t)
{
	//std::cout << x.size () << std::endl;
	
	//输入样本计算输出
	std::vector<double> out;
	output (x, out);
	for (int ii = 0 ; ii < x.size (); ++ii) {
		std::cout << x[ii] << " ";
	}
	std::cout << " : ";
	for (int ii = 0 ; ii < t.size (); ++ii) {
		std::cout << t[ii] << " ";
	}
	std::cout << " : ";
	for (int ii = 0; ii < out.size (); ++ii) {
		std::cout << out[ii] << " ";
	}	
	//std::cout << std::endl;
	//计算输出层误差
	double error = 0;
	sum_of_squares_error (out, t, error);
	e += error;
	std::cout << " : " <<  e << std::endl;
	//计算局部梯度并修正权值
	update_weights (t, out);	
	return true;
}


bool NeuralNet::train (const std::string& train_file)
{	
	std::vector<std::pair<std::vector<double>, std::vector<double>>> training_set;
	load_training_set (train_file, training_set);
	for (int i = 0; i < epoch; ++i) {
		//对训练集和随机洗牌
		shuffle (training_set);
		double e = 0;
		for (int ii = 0; ii < training_set.size (); ++ii) {
			train_step (e, training_set[ii].first, training_set[ii].second);	
		}
		printf("NeuralNet::train () : error = %f\n", e);
	}
	return true;
}


void NeuralNet::test ()
{
	using namespace std;
	//std::vector<std::pair<std::vector<double>, std::vector<double>>> t;
	//load_training_set ("test/train.txt", t);
	train ("test/train.txt");
	return;
	vector<double> x;
	for (int i = 0; i < 5; ++i) {
		x.push_back (i/5.0);
		cout << x[i] << " ";
	}
	cout << endl;
	vector<double> h1;
	propagation (x, h1, 0);
	for (int i = 0; i < h1.size (); ++i) {
		cout << h1[i] << " ";
	}
	cout << endl;
	vector<double> out;
	output (x, out);
	for (int i = 0; i < out.size (); ++i) {
		cout << out[i] << " ";
	}
	cout  << endl;
}






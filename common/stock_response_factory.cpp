#include "stdafx.h"
#include "allocator.h"
#include "stock_response_factory.h"

using namespace std;

static unsigned long request_no = 0;

string stock_response_factory::server_name;

void stock_response_factory::set_server_name(const char* _server_name) { server_name = _server_name; }

unsigned long stock_response_factory::get_request_count() { return request_no; }

void stock_response_factory::increment_request_count() 
{ 
	InterlockedIncrement(&request_no); 
	update_request_no();
}

static char our_stock_response[] =

	"HTTP/1.1 200 OK\n"
	"Date: Tue, 11 May 2013 07:28:30 GMT\n"
	"Expires: -1\n"
	"Cache-Control: private, max-age=0\n"
	"Content-Type: text/html; charset=UTF-8\n"
	"Connection: close\n\n"
	"<html>"
		"<head>"
			"<title>Stock TCP Server Response</title>"
		"</head>"
		"<body>"
			"<div style=\"text-align:center\"><h1>@@TCP_SERVER_RESPONSE</h1>"
				"<p>Hello world!</p>"

				"<p>Request no: @@_____________________ </p>"

				"<p>"
"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed quoniam et advesperascit et mihi ad villam revertendum est, nunc quidem hactenus; Quos quidem tibi studiose et diligenter tractandos magnopere censeo. Addidisti ad extremum etiam indoctum fuisse. Sed ad haec, nisi molestum est, habeo quae velim. Tollitur beneficium, tollitur gratia, quae sunt vincla concordiae. Duo Reges: constructio interrete. Est enim effectrix multarum et magnarum voluptatum. Quid vero? "

"Vide, quantum, inquam, fallare, Torquate. Laboro autem non sine causa; Terram, mihi crede, ea lanx et maria deprimet. Ita graviter et severe voluptatem secrevit a bono. Sed videbimus. Quid autem habent admirationis, cum prope accesseris? Quod autem ratione actum est, id officium appellamus. Atqui eorum nihil est eius generis, ut sit in fine atque extrerno bonorum. "

"Maximus dolor, inquit, brevis est. Quod cum dixissent, ille contra. Facile est hoc cernere in primis puerorum aetatulis. In his igitur partibus duabus nihil erat, quod Zeno commutare gestiret. "

"Ab his oratores, ab his imperatores ac rerum publicarum principes extiterunt. Quicquid porro animo cernimus, id omne oritur a sensibus; Haec quo modo conveniant, non sane intellego. Virtutis, magnitudinis animi, patientiae, fortitudinis fomentis dolor mitigari solet. Praeteritis, inquit, gaudeo. Quid, quod homines infima fortuna, nulla spe rerum gerendarum, opifices denique delectantur historia? Legimus tamen Diogenem, Antipatrum, Mnesarchum, Panaetium, multos alios in primisque familiarem nostrum Posidonium. Hoc ne statuam quidem dicturam pater aiebat, si loqui posset. "

"Scientiam pollicentur, quam non erat mirum sapientiae cupido patria esse cariorem. Quae quidem sapientes sequuntur duce natura tamquam videntes; Multa sunt dicta ab antiquis de contemnendis ac despiciendis rebus humanis; Huic mori optimum esse propter desperationem sapientiae, illi propter spem vivere. Ut necesse sit omnium rerum, quae natura vigeant, similem esse finem, non eundem. Quamquam te quidem video minime esse deterritum. An eum discere ea mavis, quae cum plane perdidiceriti nihil sciat? Non quaeritur autem quid naturae tuae consentaneum sit, sed quid disciplinae. Primum cur ista res digna odio est, nisi quod est turpis? Nosti, credo, illud: Nemo pius est, qui pietatem-; Si autem id non concedatur, non continuo vita beata tollitur. Tu autem negas fortem esse quemquam posse, qui dolorem malum putet. "

"Traditur, inquit, ab Epicuro ratio neglegendi doloris. Quid ergo aliud intellegetur nisi uti ne quae pars naturae neglegatur? Nam de isto magna dissensio est. Aliena dixit in physicis nec ea ipsa, quae tibi probarentur; Dic in quovis conventu te omnia facere, ne doleas. Beatus autem esse in maximarum rerum timore nemo potest. Quid ergo aliud intellegetur nisi uti ne quae pars naturae neglegatur? "

"Cupit enim dicere nihil posse ad beatam vitam deesse sapienti. Commoda autem et incommoda in eo genere sunt, quae praeposita et reiecta diximus; Quod ea non occurrentia fingunt, vincunt Aristonem; Etsi qui potest intellegi aut cogitari esse aliquod animal, quod se oderit? Minime vero istorum quidem, inquit. Quarum ambarum rerum cum medicinam pollicetur, luxuriae licentiam pollicetur. Scisse enim te quis coarguere possit? At enim, qua in vita est aliquid mali, ea beata esse non potest. "

"Non potes, nisi retexueris illa. Nihil minus, contraque illa hereditate dives ob eamque rem laetus. Fortitudinis quaedam praecepta sunt ac paene leges, quae effeminari virum vetant in dolore. Vos autem cum perspicuis dubia debeatis illustrare, dubiis perspicua conamini tollere. Quid, si etiam iucunda memoria est praeteritorum malorum? Occultum facinus esse potuerit, gaudebit; Cur id non ita fit? Ita ne hoc quidem modo paria peccata sunt. Qui autem de summo bono dissentit de tota philosophiae ratione dissentit. Scio enim esse quosdam, qui quavis lingua philosophari possint; "

				"</p>"
			"</div>"
		"</body>"
	"</html>";

// The test application always has the same response to an HTML request.
// (The purpose of the test code is to see how fast the socket 
//  code is, NOT to find out how quick we can parse HTML)
// This code loads the reponse string into memory, which can be the
// contents of a file specified on the command line.
// 
bool stock_response_factory::initialise(allocator_type& _allocator, const string& response_filename)
{
	stock_response = nullptr;
	stock_response_length = 0;
	if (!response_filename.empty())
	{
		load_response_from_file(response_filename, _allocator);
	}
	else
	{
		stock_response = our_stock_response;
		stock_response_length = ::strlen(stock_response);	
	}
	update_server_name();
	update_request_no();
	return (stock_response != nullptr);
}

char* stock_response_factory::stock_response = nullptr;
size_t stock_response_factory::stock_response_length = 0;

void stock_response_factory::load_response_from_file(const string& filename, allocator_type& _allocator)
{
	// load the file from disk.
	std::ifstream f(filename.c_str());
	if(f.good())
	{
		f.seekg(0, f.end);
		streampos pos = f.tellg();
		f.seekg(f.beg);

		stock_response = reinterpret_cast<char*>(_allocator.allocate(size_t(pos)));
		f.read(stock_response, pos);
	}
	
	if (stock_response != nullptr)
		stock_response_length = ::strlen(stock_response);
}

void stock_response_factory::update_server_name()
{
	const char pattern[] = "@@TCP_SERVER_RESPONSE";
	static text_substitution txt_sub(our_stock_response, our_stock_response+stock_response_length, pattern);
	txt_sub.replace(server_name.c_str(), ' ');
}

void stock_response_factory::update_request_no()
{
	const char pattern[] = "@@__________";
	static text_substitution txt_sub(our_stock_response, our_stock_response+stock_response_length, pattern);
	char buffer[22];
	_ltoa(request_no, buffer, 10);
	txt_sub.replace(buffer, '_');
}

void stock_response_factory::get_stock_response(const char*& response, size_t& response_length)
{
	response = stock_response;
	response_length = stock_response_length;
}

/**********************************************************************************************************************
** text_substitution class
***********************************************************************************************************************/
	
text_substitution::text_substitution(char* _first, char* _end, const char* _pattern)
	: first(_first),
	  end(_end),
	  pattern(_pattern)
{
	assert(first != nullptr);
	assert(end != nullptr);
	assert(pattern != nullptr);
	assert(first < end);
	pattern_len = strlen(pattern);
	assert(pattern_len != 0);
	ptr = find();
}

// yes, i know i could have used string::find but this is more fun.
char* text_substitution::find()
{
	size_t i = 1;
	for(; i<pattern_len; i++)
	{
		if(pattern[i] == pattern[0])
			break;
	}
	size_t adv = i;
	for(auto p = first; p + pattern_len <= end; p++)
	{
		if(*p == pattern[0])
		{
			auto r = &pattern[1];
			for(auto q = p+1; q < p + pattern_len; ++q, ++r)
			{
				if(*q != *r)
				{
					p += adv;
					goto skip;
				}
			}
			return p;
		}
	skip:
		;
	}
	return end;
}

void text_substitution::replace(const char* replacement, char fill_char)
{
	assert(replacement != nullptr);
	size_t replacement_len = strlen(replacement);
	assert(replacement_len != 0);
	if(ptr != end)
	{
		memset(ptr, fill_char, pattern_len);
		strncpy(ptr, replacement, std::min<size_t>(replacement_len, pattern_len));
	}
}

